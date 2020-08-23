#include <pcl/renderer/toneMapper.h>

PCL_BEGIN

ToneMapper::ToneMapper(int width, int height)
{
    width_  = static_cast<UINT>(width);
    height_ = static_cast<UINT>(height);
    initShader();
    initOutputTexture();
    initPerFrameConsts();
}

void ToneMapper::setSize(int width, int height)
{
    width_ = static_cast<UINT>(width);
    height_ = static_cast<UINT>(height);
    initOutputTexture();
}

void ToneMapper::setExposure(float exposure)
{
    perFrame_.update({ exposure, { 0, 0, 0 } });
}

ComPtr<ID3D11ShaderResourceView> ToneMapper::getOutput() const
{
    return outputSRV_;
}

void ToneMapper::render(ComPtr<ID3D11ShaderResourceView> hdrImg)
{
    hdrImgSlot_->setShaderResourceView(hdrImg);

    shader_.bind();
    rscMgr_.bind();
    d3d11::gDeviceContext->Dispatch(width_, height_, 1);
    rscMgr_.unbind();
    shader_.unbind();
}

void ToneMapper::initShader()
{
    shader_.initializeStageFromFile<d3d11::CS>("./asset/tonemap.hlsl");
    rscMgr_ = shader_.createResourceManager();

    hdrImgSlot_ = rscMgr_.getShaderResourceViewSlot<d3d11::CS>("HDRImage");
}

void ToneMapper::initOutputTexture()
{
    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width          = width_;
    texDesc.Height         = height_;
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    ComPtr<ID3D11Texture2D> tex;
    PCL_THROW_IF_FAILED(
        d3d11::gDevice->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf()),
        "failed to create tone mapping output texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> srv;
    PCL_THROW_IF_FAILED(d3d11::gDevice->CreateShaderResourceView(
        tex.Get(), &srvDesc, srv.GetAddressOf()),
        "failed to create tone mapping srv");

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    ComPtr<ID3D11UnorderedAccessView> uav;
    PCL_THROW_IF_FAILED(d3d11::gDevice->CreateUnorderedAccessView(
        tex.Get(), &uavDesc, uav.GetAddressOf()),
        "failed to create tone mapping uav");

    rscMgr_.getUnorderedAccessViewSlot<d3d11::CS>("Output")
        ->setUnorderedAccessView(uav);

    outputTex_.Swap(tex);
    outputSRV_.Swap(srv);
    outputUAV_.Swap(uav);
}

void ToneMapper::initPerFrameConsts()
{
    perFrame_.initialize();
    setExposure(1);
    rscMgr_.getConstantBufferSlot<d3d11::CS>("PerFrame")
        ->setBuffer(perFrame_);
}

PCL_END

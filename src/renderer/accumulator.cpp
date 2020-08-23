#include <pcl/renderer/accumulator.h>

PCL_BEGIN

Accumulator::Accumulator(int width, int height)
    : width_(static_cast<UINT>(width)),
      height_(static_cast<UINT>(height)),
      historySlot_(nullptr),
      newFrameSlot_(nullptr),
      outputSlot_(nullptr),
      accumulatedCount_(0)
{
    initShader();
    initPingPongTextures();
    initPerFrameConsts();
}

void Accumulator::setSize(int width, int height)
{
    width_  = width;
    height_ = height;

    initPingPongTextures();
    clearHistory();
}

void Accumulator::clearHistory()
{
    accumulatedCount_ = 0;
}

void Accumulator::addNewFrame(ComPtr<ID3D11ShaderResourceView> srv)
{
    const float totalCount     = static_cast<float>(accumulatedCount_ + 1);
    const float historyWeight  = accumulatedCount_ / totalCount;
    const float newFrameWeight = 1 / totalCount;

    perFrame_.update({
        historyWeight,
        newFrameWeight,
        { 0, 0 }
    });

    historySlot_->setShaderResourceView(accumulated_.srv);
    newFrameSlot_->setShaderResourceView(srv);
    outputSlot_->setUnorderedAccessView(nextTarget_.uav);

    shader_.bind();
    rscMgr_.bind();
    d3d11::gDeviceContext->Dispatch(width_, height_, 1);
    rscMgr_.unbind();
    shader_.unbind();

    ++accumulatedCount_;
    std::swap(accumulated_, nextTarget_);
}

ComPtr<ID3D11ShaderResourceView> Accumulator::getAccumulatedOutput() const
{
    return accumulated_.srv;
}

int Accumulator::getAccumulatedFrameCount() const noexcept
{
    return accumulatedCount_;
}

void Accumulator::initShader()
{
    shader_.initializeStageFromFile<d3d11::CS>("./asset/accumulate.hlsl");
    rscMgr_ = shader_.createResourceManager();

    historySlot_  = rscMgr_.getShaderResourceViewSlot<d3d11::CS>("History");
    newFrameSlot_ = rscMgr_.getShaderResourceViewSlot<d3d11::CS>("NewFrame");
    outputSlot_   = rscMgr_.getUnorderedAccessViewSlot<d3d11::CS>("Output");
}

void Accumulator::initPingPongTextures()
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

    ComPtr<ID3D11Texture2D> historyTex, outputTex;
    if(FAILED(d3d11::gDevice->CreateTexture2D(
        &texDesc, nullptr, historyTex.GetAddressOf())))
        throw PCLException("Accumulator: failed to create history texture");
    if(FAILED(d3d11::gDevice->CreateTexture2D(
        &texDesc, nullptr, outputTex.GetAddressOf())))
        throw PCLException("Accumulator: failed to create output texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> historySRV, outputSRV;
    if(FAILED(d3d11::gDevice->CreateShaderResourceView(
        historyTex.Get(), &srvDesc, historySRV.GetAddressOf())))
        throw PCLException("Accumulator: failed to create history srv");
    if(FAILED(d3d11::gDevice->CreateShaderResourceView(
        outputTex.Get(), &srvDesc, outputSRV.GetAddressOf())))
        throw PCLException("Accumulator: failed to create output srv");

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    ComPtr<ID3D11UnorderedAccessView> historyUAV, outputUAV;
    if(FAILED(d3d11::gDevice->CreateUnorderedAccessView(
        historyTex.Get(), &uavDesc, historyUAV.GetAddressOf())))
        throw PCLException("Accumulator: failed to create history uav");
    if(FAILED(d3d11::gDevice->CreateUnorderedAccessView(
        outputTex.Get(), &uavDesc, outputUAV.GetAddressOf())))
        throw PCLException("Accumulator: failed to create output uav");

    accumulated_ = { std::move(historySRV), std::move(historyUAV) };
    nextTarget_  = { std::move(outputSRV),  std::move(outputUAV)  };
}

void Accumulator::initPerFrameConsts()
{
    perFrame_.initialize();
    rscMgr_.getConstantBufferSlot<d3d11::CS>("PerFrame")->setBuffer(perFrame_);
}

PCL_END

#include <pcl/renderer/jensenRhoDt.h>
#include <pcl/renderer/tracer.h>

PCL_BEGIN

Tracer::Tracer(
    const Int2 &outputSize,
    const Int3 &paperSize,
    float       paperDistance,
    int         spp)
    : outputSize_(outputSize), paperSize_(paperSize),
      paperDistance_(paperDistance), backLightDistance_(paperDistance),
      spp_(spp), envLight_(0.15f, 0.15f, 0.15f), eyeZ_(-1)
{
    initShader();
    initRenderTarget();
    initPerFrameConstantBuffer();
    initPaperTexture();
    initPaperMaterials();
    initBackLightTexture();
    initRNGTexture();
    initJensenRhoDt();
    setResourceBindings();
}

void Tracer::setPaperSize(const Int3 &paperSize)
{
    paperSize_ = paperSize;
    initPaperTexture();
    initPaperMaterials();
    initBackLightTexture();
    setResourceBindings();
}

void Tracer::setOutputSize(const Int2 &newOutputSize)
{
    if(newOutputSize != outputSize_)
    {
        outputSize_ = newOutputSize;
        initRenderTarget();
        initRNGTexture();
        setResourceBindings();
    }
}

void Tracer::setSPP(int spp) noexcept
{
    spp_ = spp;
}

void Tracer::setPaperData(int z, const Texel *data)
{
    const UINT subrscIdx = D3D11CalcSubresource(0, static_cast<UINT>(z), 1);
    d3d11::gDeviceContext->UpdateSubresource(
        papersTex_.Get(), subrscIdx, nullptr,
        data, sizeof(Texel) * paperSize_.x, 0);
}

void Tracer::setPaperDiffuse(int z, float reflectionRatio)
{
    PaperMaterial paperMaterial{};
    paperMaterial.type = PaperMaterial::TYPE_DIFFUSE;
    paperMaterial.m01  = reflectionRatio;

    D3D11_BOX box;
    box.left   = static_cast<UINT>(sizeof(PaperMaterial) * z);
    box.right  = static_cast<UINT>(sizeof(PaperMaterial) * (z + 1));
    box.top    = 0;
    box.bottom = 1;
    box.front  = 0;
    box.back   = 1;

    d3d11::gDeviceContext->UpdateSubresource(
        paperMaterialsBuf_.Get(), 0, &box, &paperMaterial, 0, 0);
}

namespace
{
    std::pair<Float3, Float3> computeRdAndTd(
        int n, const Float3 &color,
        float sigma_s, float sigma_a, float d,
        float gf, float gb, float wf, float wb) noexcept
    {
        const float sigma_s_bar = (1 - (wb * gb + wf * gf)) * sigma_s;
        const float sigma_t_bar = sigma_a + sigma_s_bar;
        const float alpha_bar = sigma_s_bar / sigma_t_bar;
        const float sigma_tr = std::sqrt(3 * sigma_a * sigma_t_bar);

        auto sign = [](float x) { return x > 0 ? 1.0f : -1.0f; };

        const float l = 1 / sigma_t_bar;

        const Float3 A = (Float3(1) + color) / (Float3(float(1.001)) - color);

        const float D = 1 / (3 * sigma_t_bar);

        const Float3 zb = 2 * A * D;

        auto z_r_i = [&](int i) { return float(2) * i * (d + zb + zb) + l; };
        auto z_v_i = [&](int i) { return float(2) * i * (d + zb + zb) - l - 2 * zb; };

        Float3 Rd, Td;
        for(int i = -n; i <= n; ++i)
        {
            const Float3 zri = z_r_i(i);
            const Float3 zvi = z_v_i(i);

            for(int c = 0; c < 3; ++c)
            {
                Rd[c] +=
                    sign(zri[c]) * std::exp(-sigma_tr * std::abs(zri[c]))
                  - sign(zvi[c]) * std::exp(-sigma_tr * std::abs(zvi[c]));

                Td[c] +=
                    sign(d - zri[c]) * std::exp(-sigma_tr * std::abs(d - zri[c]))
                  - sign(d - zvi[c]) * std::exp(-sigma_tr * std::abs(d - zvi[c]));
            }
        }
        Rd *= alpha_bar / 2;
        Td *= alpha_bar / 2;

        return { Rd, Td };
    }
}

void Tracer::setPaperJensen(
    int z,
    float gf, float gb, float wf, float wb,
    float frontEta, float backEta,
    float frontM, float backM,
    float d, float sigmaS, float sigmaA,
    const Float3 &diffusionAlbedo)
{
    d = (std::max)(0.05f, d);

    const float sigmaT = sigmaS + sigmaA;
    const float alpha  = sigmaS / sigmaT;
    const float tauD   = d * sigmaT;

    const auto [Rd, Td] = computeRdAndTd(
        4, diffusionAlbedo, sigmaS, sigmaA, d, gf, gb, wf, wb);

    PaperMaterial material{};
    material.type = PaperMaterial::TYPE_JENSEN;
    material.m01 = frontEta;
    material.m02 = backEta;
    material.m03 = (std::max)(0.01f, frontM);
    material.m04 = (std::max)(0.01f, backM);
    material.m05 = Rd.x;
    material.m06 = Rd.y;
    material.m07 = Rd.z;
    material.m08 = Td.x;
    material.m09 = Td.y;
    material.m10 = Td.z;
    material.m11 = alpha;
    material.m12 = tauD;
    material.m13 = wf;
    material.m14 = gf;
    material.m15 = wb;
    material.m16 = gb;

    D3D11_BOX box;
    box.left = static_cast<UINT>(sizeof(PaperMaterial) * z);
    box.right = static_cast<UINT>(sizeof(PaperMaterial) * (z + 1));
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;

    d3d11::gDeviceContext->UpdateSubresource(
        paperMaterialsBuf_.Get(), 0, &box, &material, 0, 0);
}

void Tracer::setBackLightRadiance(const agz::math::color3f *data)
{
    const int texelCount = paperSize_.x * paperSize_.y;
    std::vector<float> initData(paperSize_.x * paperSize_.y * 4);
    for(int i = 0, j = 0; i < texelCount; ++i, j += 4)
    {
        initData[j + 0] = data[i].r;
        initData[j + 1] = data[i].g;
        initData[j + 2] = data[i].b;
        initData[j + 3] = 0;
    }
    d3d11::gDeviceContext->UpdateSubresource(
        backLightTex_.Get(), 0, nullptr,
        initData.data(), sizeof(float) * 4 * paperSize_.x, 0);
}

void Tracer::setPaperDistance(float distance) noexcept
{
    paperDistance_ = distance;
}

void Tracer::setBackLightDistance(float distance) noexcept
{
    backLightDistance_ = distance;
}

void Tracer::setEnvLight(const Float3 &envLight) noexcept
{
    envLight_ = envLight;
}

void Tracer::setEyeZ(float z) noexcept
{
    eyeZ_ = z;
}

void Tracer::render() const
{
    perFrame_.update({
        static_cast<uint32_t>(outputSize_.x),
        static_cast<uint32_t>(outputSize_.y),
        static_cast<uint32_t>(paperSize_.z),
        static_cast<uint32_t>(spp_),
        static_cast<uint32_t>(paperSize_.x),
        static_cast<uint32_t>(paperSize_.y),
        paperDistance_,
        backLightDistance_,
        envLight_,
        eyeZ_
    });

    tracingShader_.bind();
    tracingResources_.bind();

    d3d11::gDeviceContext->Dispatch(
        static_cast<UINT>(outputSize_.x), static_cast<UINT>(outputSize_.y), 1);

    tracingResources_.unbind();
    tracingShader_.unbind();
}

ComPtr<ID3D11ShaderResourceView> Tracer::getOutput() const
{
    return outputSRV_;
}

void Tracer::initShader()
{
    const D3D_SHADER_MACRO macros[2] = {
        { "MAX_DEPTH", "20" },
        { nullptr, nullptr }
    };

    tracingShader_.initializeStageFromFile<d3d11::CS>(
        "./asset/tracing.hlsl", macros);

    tracingResources_ = tracingShader_.createResourceManager();
}

void Tracer::initRenderTarget()
{
    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width              = static_cast<UINT>(outputSize_.x);
    texDesc.Height             = static_cast<UINT>(outputSize_.y);
    texDesc.MipLevels          = 1;
    texDesc.ArraySize          = 1;
    texDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc.Count   = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage              = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags          = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags     = 0;
    texDesc.MiscFlags          = 0;

    ComPtr<ID3D11Texture2D> tex;
    if(FAILED(d3d11::gDevice->CreateTexture2D(
        &texDesc, nullptr, tex.GetAddressOf())))
        throw PCLException("Tracer: failed to create output texture");

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    ComPtr<ID3D11UnorderedAccessView> uav;
    if(FAILED(d3d11::gDevice->CreateUnorderedAccessView(
        tex.Get(), &uavDesc, uav.GetAddressOf())))
        throw PCLException("Tracer: failed to create output uav");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> srv;
    if(FAILED(d3d11::gDevice->CreateShaderResourceView(
        tex.Get(), &srvDesc, srv.GetAddressOf())))
        throw PCLException("failed to create output srv");

    outputTex_.Swap(tex);
    outputUAV_.Swap(uav);
    outputSRV_.Swap(srv);
}

void Tracer::initPerFrameConstantBuffer()
{
    perFrame_.initialize();
}

void Tracer::initPaperTexture()
{
    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width          = static_cast<UINT>(paperSize_.x);
    texDesc.Height         = static_cast<UINT>(paperSize_.y);
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = static_cast<UINT>(paperSize_.z);
    texDesc.Format         = DXGI_FORMAT_R8G8B8A8_UINT;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    ComPtr<ID3D11Texture2D> tex;
    if(FAILED(d3d11::gDevice->CreateTexture2D(
        &texDesc, nullptr, tex.GetAddressOf())))
        throw PCLException("Tracer: failed to create paper texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                         = DXGI_FORMAT_R8G8B8A8_UINT;
    srvDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MipLevels       = 1;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize       = static_cast<UINT>(paperSize_.z);

    ComPtr<ID3D11ShaderResourceView> srv;
    if(FAILED(d3d11::gDevice->CreateShaderResourceView(
        tex.Get(), &srvDesc, srv.GetAddressOf())))
        throw PCLException("Tracer: failed to create paper srv");

    papersTex_.Swap(tex);
    papersSRV_.Swap(srv);
}

void Tracer::initPaperMaterials()
{
    const UINT byteSize = static_cast<UINT>(
        sizeof(PaperMaterial) * paperSize_.z);

    D3D11_BUFFER_DESC bufDesc;
    bufDesc.ByteWidth           = byteSize;
    bufDesc.Usage               = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    bufDesc.CPUAccessFlags      = 0;
    bufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufDesc.StructureByteStride = sizeof(PaperMaterial);

    ComPtr<ID3D11Buffer> buf;
    if(FAILED(d3d11::gDevice->CreateBuffer(
        &bufDesc, nullptr, buf.GetAddressOf())))
        throw PCLException("Tracer: failed to create paper material buf");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format               = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement  = 0;
    srvDesc.Buffer.NumElements   = static_cast<UINT>(paperSize_.z);

    ComPtr<ID3D11ShaderResourceView> srv;
    if(FAILED(d3d11::gDevice->CreateShaderResourceView(
        buf.Get(), &srvDesc, srv.GetAddressOf())))
        throw PCLException("Tracer: failed to create paper material srv");

    paperMaterialsBuf_.Swap(buf);
    paperMaterialsSRV_.Swap(srv);
}

void Tracer::initBackLightTexture()
{
    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width          = static_cast<UINT>(paperSize_.x);
    texDesc.Height         = static_cast<UINT>(paperSize_.y);
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    ComPtr<ID3D11Texture2D> tex;
    if(FAILED(d3d11::gDevice->CreateTexture2D(
        &texDesc, nullptr, tex.GetAddressOf())))
        throw PCLException("Tracer: failed to create backlight texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> srv;
    if(FAILED(d3d11::gDevice->CreateShaderResourceView(
        tex.Get(), &srvDesc, srv.GetAddressOf())))
        throw PCLException("Tracer: failed to create backlight srv");

    backLightTex_.Swap(tex);
    backLightSRV_.Swap(srv);
}

void Tracer::initRNGTexture()
{
    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width          = static_cast<UINT>(outputSize_.x);
    texDesc.Height         = static_cast<UINT>(outputSize_.y);
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32_UINT;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_UNORDERED_ACCESS;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    const int outputTexelCount = outputSize_.x * outputSize_.y;
    std::vector<uint32_t> initData(outputTexelCount);
    for(int i = 0; i < outputTexelCount; ++i)
        initData[i] = static_cast<uint32_t>(i * i + 1);

    D3D11_SUBRESOURCE_DATA subrscData;
    subrscData.pSysMem          = initData.data();
    subrscData.SysMemPitch      = static_cast<UINT>(sizeof(uint32_t)
                                                  * texDesc.Width);
    subrscData.SysMemSlicePitch = 0;

    ComPtr<ID3D11Texture2D> tex;
    if(FAILED(d3d11::gDevice->CreateTexture2D(
        &texDesc, &subrscData, tex.GetAddressOf())))
        throw PCLException("Tracer: failed to create RNG texture");

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format             = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    ComPtr<ID3D11UnorderedAccessView> uav;
    if(FAILED(d3d11::gDevice->CreateUnorderedAccessView(
        tex.Get(), &uavDesc, uav.GetAddressOf())))
        throw PCLException("failed to create RNG uav");

    RNGTex_.Swap(tex);
    RNGUAV_.Swap(uav);
}

void Tracer::initJensenRhoDt()
{
    jensenRhoDt_ = loadJensenRhoDt();

    D3D11_SAMPLER_DESC samplerState;
    samplerState.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerState.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerState.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerState.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerState.MipLODBias     = 0;
    samplerState.MaxAnisotropy  = 1;
    samplerState.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerState.BorderColor[0] = 0;
    samplerState.BorderColor[1] = 0;
    samplerState.BorderColor[2] = 0;
    samplerState.BorderColor[3] = 0;
    samplerState.MinLOD         = FLT_MAX;
    samplerState.MaxLOD         = FLT_MAX;

    if(FAILED(d3d11::gDevice->CreateSamplerState(
        &samplerState, jensenLinearSampler_.GetAddressOf())))
        throw PCLException("failed to create jensen linear sampler");
}

void Tracer::setResourceBindings()
{
    tracingResources_.getConstantBufferSlot<d3d11::CS>("PerFrame")
        ->setBuffer(perFrame_);
    tracingResources_.getUnorderedAccessViewSlot<d3d11::CS>("RNGState")
        ->setUnorderedAccessView(RNGUAV_);
    tracingResources_.getShaderResourceViewSlot<d3d11::CS>("PaperMaterials")
        ->setShaderResourceView(paperMaterialsSRV_);
    tracingResources_.getShaderResourceViewSlot<d3d11::CS>("Papers")
        ->setShaderResourceView(papersSRV_);
    tracingResources_.getShaderResourceViewSlot<d3d11::CS>("BackLight")
        ->setShaderResourceView(backLightSRV_);
    tracingResources_.getShaderResourceViewSlot<d3d11::CS>("JensenRhoDt")
        ->setShaderResourceView(jensenRhoDt_);
    tracingResources_.getUnorderedAccessViewSlot<d3d11::CS>("Output")
        ->setUnorderedAccessView(outputUAV_);
    tracingResources_.getSamplerSlot<d3d11::CS>("JensenLinearSampler")
        ->setSampler(jensenLinearSampler_);
}

PCL_END

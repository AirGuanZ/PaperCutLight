#pragma once

#include <pcl/common.h>

PCL_BEGIN

class Tracer : public agz::misc::uncopyable_t
{
public:

    struct Texel
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;

        uint8_t binary;
    };

    Tracer(
        const Int2 &outputSize,
        const Int3 &paperSize,
        float       paperDistance,
        int         spp);

    void setPaperSize(const Int3 &paperSize);

    void setOutputSize(const Int2 &newOutputSize);

    void setSPP(int spp) noexcept;

    void setPaperData(int z, const Texel *data);

    void setBackLightRadiance(const agz::math::color3f *data);

    void setPaperDiffuse(int z, float reflectionRatio);

    void setPaperJensen(
        int z,
        float gf, float gb, float wf, float wb,
        float frontEta, float backEta,
        float frontM, float backM,
        float d, float sigmaS, float sigmaA,
        const Float3 &diffusionAlbedo);

    void setPaperDistance(float distance) noexcept;

    void setBackLightDistance(float distance) noexcept;

    void setEnvLight(const Float3 &envLight) noexcept;

    void setEyeZ(float z) noexcept;

    void render() const;

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

private:

    void initShader();

    void initRenderTarget();

    void initPerFrameConstantBuffer();

    void initPaperTexture();

    void initPaperMaterials();

    void initBackLightTexture();

    void initRNGTexture();

    void initJensenRhoDt();

    void setResourceBindings();

    struct PerFrame
    {
        uint32_t outputWidth;
        uint32_t outputHeight;
        uint32_t paperCount;
        uint32_t spp;
        uint32_t paperWidth;
        uint32_t paperHeight;
        float    paperDistance;
        float    backLightDistance;
        Float3   envLight;
        float    eyeZ;
    };

    struct PaperMaterial
    {
        static const uint32_t TYPE_DIFFUSE = 1;
        static const uint32_t TYPE_JENSEN  = 2;

        uint32_t type; // m00
        float m01, m02, m03;
        float m04, m05, m06, m07;
        float m08, m09, m10, m11;
        float m12, m13, m14, m15;
        float m16, m17, m18, m19;
    };

    Int2  outputSize_;
    Int3  paperSize_;
    float paperDistance_;
    float backLightDistance_;
    int   spp_;

    Float3 envLight_;
    float eyeZ_;

    d3d11::Shader<d3d11::CS>          tracingShader_;
    d3d11::ResourceManager<d3d11::CS> tracingResources_;

    mutable d3d11::ConstantBuffer<PerFrame> perFrame_;

    ComPtr<ID3D11Texture2D>           RNGTex_;
    ComPtr<ID3D11UnorderedAccessView> RNGUAV_;

    ComPtr<ID3D11Buffer>             paperMaterialsBuf_;
    ComPtr<ID3D11ShaderResourceView> paperMaterialsSRV_;

    ComPtr<ID3D11Texture2D>          papersTex_;
    ComPtr<ID3D11ShaderResourceView> papersSRV_;

    ComPtr<ID3D11Texture2D>          backLightTex_;
    ComPtr<ID3D11ShaderResourceView> backLightSRV_;

    ComPtr<ID3D11Texture2D>           outputTex_;
    ComPtr<ID3D11UnorderedAccessView> outputUAV_;
    ComPtr<ID3D11ShaderResourceView>  outputSRV_;

    ComPtr<ID3D11ShaderResourceView> jensenRhoDt_;
    ComPtr<ID3D11SamplerState> jensenLinearSampler_;
};

PCL_END

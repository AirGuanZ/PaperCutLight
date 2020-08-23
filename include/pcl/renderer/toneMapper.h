#pragma once

#include <pcl/common.h>

PCL_BEGIN

class ToneMapper : public agz::misc::uncopyable_t
{
public:

    ToneMapper(int width, int height);

    void setSize(int width, int height);

    void setExposure(float exposure);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

    void render(ComPtr<ID3D11ShaderResourceView> hdrImg);

private:

    struct PerFrame
    {
        float exposure = 1;
        float pad[3] = { 0 };
    };

    void initShader();

    void initOutputTexture();

    void initPerFrameConsts();

    UINT width_;
    UINT height_;

    d3d11::Shader<d3d11::CS>          shader_;
    d3d11::ResourceManager<d3d11::CS> rscMgr_;

    d3d11::ShaderResourceViewSlot<d3d11::CS> *hdrImgSlot_;

    ComPtr<ID3D11Texture2D>           outputTex_;
    ComPtr<ID3D11ShaderResourceView>  outputSRV_;
    ComPtr<ID3D11UnorderedAccessView> outputUAV_;

    d3d11::ConstantBuffer<PerFrame> perFrame_;
};

PCL_END

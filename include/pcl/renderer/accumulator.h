#pragma once

#include <pcl/common.h>

PCL_BEGIN

class Accumulator : public agz::misc::uncopyable_t
{
public:

    Accumulator(int width, int height);

    void setSize(int width, int height);

    void clearHistory();

    void addNewFrame(ComPtr<ID3D11ShaderResourceView> srv);

    ComPtr<ID3D11ShaderResourceView> getAccumulatedOutput() const;

    int getAccumulatedFrameCount() const noexcept;

private:

    void initShader();

    void initPingPongTextures();

    void initPerFrameConsts();

    struct PerFrame
    {
        float historyWeight;
        float newFrameWeight;
        float pad0[2];
    };

    UINT width_;
    UINT height_;

    d3d11::Shader<d3d11::CS>          shader_;
    d3d11::ResourceManager<d3d11::CS> rscMgr_;

    d3d11::ShaderResourceViewSlot<d3d11::CS>  *historySlot_;
    d3d11::ShaderResourceViewSlot<d3d11::CS>  *newFrameSlot_;
    d3d11::UnorderedAccessViewSlot<d3d11::CS> *outputSlot_;

    d3d11::ConstantBuffer<PerFrame> perFrame_;

    struct Buffer
    {
        ComPtr<ID3D11ShaderResourceView>  srv;
        ComPtr<ID3D11UnorderedAccessView> uav;
    };

    Buffer accumulated_;
    Buffer nextTarget_;

    int accumulatedCount_;
};

PCL_END

#define THREAD_GROUP_WIDTH  16
#define THREAD_GROUP_HEIGHT 16

cbuffer PerFrame
{
    float HistoryWeight;
    float NewFrameWeight;
};

Texture2D<float4> History;
Texture2D<float4> NewFrame;

RWTexture2D<float4> Output;

[numthreads(THREAD_GROUP_WIDTH, THREAD_GROUP_HEIGHT, 1)]
void main(int3 threadIdx : SV_DispatchThreadID)
{
    float4 history  = History[threadIdx.xy];
    float4 newFrame = NewFrame[threadIdx.xy];
    Output[threadIdx.xy] = HistoryWeight * history + NewFrameWeight * newFrame;
}

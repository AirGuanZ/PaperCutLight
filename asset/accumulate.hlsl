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
    float4 history  = pow(History[threadIdx.xy], 2.2);
    float4 newFrame = NewFrame[threadIdx.xy];
    float4 linearVal = HistoryWeight * history + NewFrameWeight * newFrame;
    Output[threadIdx.xy] = pow(linearVal, 1 / 2.2);
}

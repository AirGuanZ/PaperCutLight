#define THREAD_GROUP_WIDTH  16
#define THREAD_GROUP_HEIGHT 16

#define A 2.51
#define B 0.03
#define C 2.43
#define D 0.59
#define E 0.14

cbuffer PerFrame
{
    float Exposure;
};

Texture2D<float4> HDRImage;
RWTexture2D<float4> Output;

[numthreads(THREAD_GROUP_WIDTH, THREAD_GROUP_HEIGHT, 1)]
void main(int3 threadIdx : SV_DispatchThreadID)
{
    float3 input = Exposure * HDRImage[threadIdx.xy].rgb;
    float3 output = (input * (A * input + B)) / (input * (C * input + D) + E);
    Output[threadIdx.xy] = float4(saturate(pow(output, 1 / 2.2)), 1);
}

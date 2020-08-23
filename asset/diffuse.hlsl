#ifndef PCL_DIFFUSE
#define PCL_DIFFUSE

#include "random.hlsl"

#define PI 3.1415926535

float3 diffuseSampleZWeightedHemisphere(inout uint rngState)
{
    float u1 = 2 * randomFloat(rngState) - 1;
    float u2 = 2 * randomFloat(rngState) - 1;
    float2 sam = float2(0, 0);

    if(u1 != 0 || u2 != 0)
    {
        float theta, r;
        if(abs(u1) > abs(u2))
        {
            r = u1;
            theta = 0.25 * PI * (u2 / u1);
        }
        else
        {
            r = u2;
            theta = 0.5 * PI - 0.25 * PI * (u1 / u2);
        }
        sam = r * float2(cos(theta), sin(theta));
    }

    float z = sqrt(max(0, 1 - dot(sam, sam)));
    return float3(sam, z);
}

#define DIFFUSE_REFL_PDF 0.85

void sampleDiffuse(
    float reflectionRatio,
    bool isFront, float3 wo,
    inout uint rngState,
    out float3 coef, out float3 wi)
{
    float3 samDir = diffuseSampleZWeightedHemisphere(rngState);

    bool isReflection = randomFloat(rngState) < DIFFUSE_REFL_PDF;
    if(isFront == isReflection)
        samDir.z = -samDir.z;
    
    wi = samDir;

    float v = isReflection ? reflectionRatio / DIFFUSE_REFL_PDF :
                             (1 - reflectionRatio) / (1 - DIFFUSE_REFL_PDF);
    coef = float3(v, v, v);
}

#endif // #ifndef PCL_DIFFUSE

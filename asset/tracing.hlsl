// #define MAX_DEPTH 20

#include "diffuse.hlsl"
#include "jensen.hlsl"
#include "random.hlsl"

#define THREAD_GROUP_WIDTH 8
#define THREAD_GROUP_HEIGHT 4
#define EPS 0.01

cbuffer PerFrame
{
    uint OutputWidth;
    uint OutputHeight;
    uint PaperCount;
    uint SPP;

    uint PaperWidth;
    uint PaperHeight;
    float PaperDistance;
    float BackLightDistance;

    float3 EnvLight;
    float EyeZ;
};

struct PaperMaterial
{
    uint  m00;
    float m01;
    float m02;
    float m03;
    float m04;
    float m05;
    float m06;
    float m07;
    float m08;
    float m09;
    float m10;
    float m11;
    float m12;
    float m13;
    float m14;
    float m15;
    float m16;
    float m17;
    float m18;
    float m19;
};

StructuredBuffer<PaperMaterial> PaperMaterials;

// 0.rgb: color
// 0.a  : binary
Texture2DArray<uint4> Papers;

Texture2D<float4> BackLight;

RWTexture2D<float4> Output;

float findIntersectionT(float3 rayOri, float3 rayDir, float planeZ)
{
    if(planeZ < -0.5 || planeZ >= PaperCount + 0.5)
        return -1;
    float dz = PaperDistance * planeZ - rayOri.z;
    if(planeZ >= PaperCount - 0.5)
        dz += BackLightDistance - PaperDistance;
    return dz / rayDir.z;
}

void generateCameraRay(int2 xy, out float3 ori, out float3 dir)
{
    if(EyeZ >= 0)
    {
        ori = float3(xy.x + 0.5, xy.y + 0.5, -1);
        dir = float3(0, 0, 1);
    }
    else
    {
        ori = float3(0.5 * OutputWidth, 0.5 * OutputHeight, EyeZ * OutputWidth);
        dir = normalize(float3(xy.x + 0.5, xy.y + 0.5, 0) - ori);
    }
}

float4 trace(int3 threadIndex, inout uint rngState)
{
    float3 rayOri, rayDir;
    generateCameraRay(threadIndex.xy, rayOri, rayDir);
    float  nextPlaneZ = 0;

    float3 coef = float3(1, 1, 1);

    for(int depth = 1, scatterDepth = 0; depth <= MAX_DEPTH; ++depth)
    {
        // find next intersection

        float t = findIntersectionT(rayOri, rayDir, nextPlaneZ);
        if(t <= EPS)
            return float4(EnvLight * coef, depth == 1 ? 0 : 1);

        float3 inct = rayOri + t * rayDir;
        if(inct.x < 0 || inct.y < 0 ||
           inct.x > float(OutputWidth) ||
           inct.y > float(OutputHeight))
        {
            if(scatterDepth > 0)
            {
                /*inct.x = max(0, min(OutputWidth, inct.x));
                inct.y = max(0, min(OutputHeight, inct.y));*/
                return float4(0, 0, 0, 1);
            }
            else
            {
                bool ox = threadIndex.x / 8 % 2 == 0;
                bool oy = threadIndex.y / 8 % 2 == 0;
                float v = ox ^ oy ? 0.4 : 0.1;
                return float4(v, v, v, 0);
            }
        }
        
        bool isFront = rayDir.z > 0;

        // material color & binary

        float2 uv = float2(
            inct.x / OutputWidth, inct.y / OutputHeight);
            
        int paperX = min(int(uv.x * PaperWidth), PaperWidth - 1);
        int paperY = min(int(uv.y * PaperHeight), PaperHeight - 1);
        int paperZ = int(nextPlaneZ);

        if(paperZ >= int(PaperCount))
        {
            float4 lightRad = BackLight[int2(paperX, paperY)];
            return float4(coef * lightRad.rgb, 1);
        }

        uint4 paperPoint = Papers[int3(paperX, paperY, paperZ)];
        uint binary = paperPoint.a;

        if(binary == 0)
        {
            rayOri = inct + float3(0, 0, rayDir.z > 0 ? EPS : -EPS);
            nextPlaneZ += rayDir.z > 0 ? 1 : -1;
            continue;
        }

        ++scatterDepth;

        // sample bsdf

        PaperMaterial paperMaterial = PaperMaterials[int(nextPlaneZ)];
        uint materialType = paperMaterial.m00;

        float3 dir, throughput;
        if(materialType == 0)
        {
            rayOri = inct + float3(0, 0, rayDir.z > 0 ? EPS : -EPS);
            nextPlaneZ += rayDir.z > 0 ? 1 : -1;
            continue;
        }
        else if(materialType == 1)
        {
            // diffuse

            float reflRatio = paperMaterial.m01;
            sampleDiffuse(
                reflRatio, isFront, normalize(-rayDir),
                rngState, throughput, dir);
        }
        else if(materialType == 2)
        {
            // jensen

            JensenParams params;
            params.etaFront = paperMaterial.m01;
            params.etaBack  = paperMaterial.m02;
            params.mFront   = paperMaterial.m03;
            params.mBack    = paperMaterial.m04;
            params.Rd.r     = paperMaterial.m05;
            params.Rd.g     = paperMaterial.m06;
            params.Rd.b     = paperMaterial.m07;
            params.Td.r     = paperMaterial.m08;
            params.Td.g     = paperMaterial.m09;
            params.Td.b     = paperMaterial.m10;
            params.alpha    = paperMaterial.m11;
            params.tauD     = paperMaterial.m12;
            params.HGArg.r  = paperMaterial.m13;
            params.HGArg.g  = paperMaterial.m14;
            params.HGArg.b  = paperMaterial.m15;
            params.HGArg.a  = paperMaterial.m16;

            sampleJensen(params, normalize(-rayDir), rngState, throughput, dir);
        }
        else
            return float4(0, 0, 0, 1);

        // next ray

        float3 color = paperPoint.rgb * float3(1 / 255.0, 1 / 255.0, 1 / 255.0);

        coef *= color * throughput;
        rayDir = dir;
        rayOri = inct + float3(0, 0, dir.z > 0 ? EPS : -EPS);
        nextPlaneZ += rayDir.z > 0 ? 1 : -1;
    }

    return float4(EnvLight * coef, 1);
}

[numthreads(THREAD_GROUP_WIDTH, THREAD_GROUP_HEIGHT, 1)]
void main(int3 threadIndex : SV_DispatchThreadID)
{
    uint rngState = loadRNG(threadIndex.xy);
    
    float4 sum = float4(0, 0, 0, 0);
    for(uint i = 0; i < SPP; ++i)
    {
        float4 single = trace(threadIndex, rngState);
        if(!any(isinf(single) | isnan(single)))
            sum += single;
    }
        
    storeRNG(threadIndex.xy, rngState);
    Output[threadIndex.xy] = float4(sum.rgb / SPP, 1);
}

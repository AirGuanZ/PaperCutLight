#ifndef PCL_JENSEN
#define PCL_JENSEN

#define JENSEN_EPS 0.01

#include "diffuse.hlsl"

Texture3D<float> JensenRhoDt;
SamplerState JensenLinearSampler;

float jensenSqr(float x) { return x * x; }

float jensenDGGX(float cosThetaH, float m)
{
    return m * m /
        (PI * jensenSqr(1 + (jensenSqr(m) - 1) * jensenSqr(cosThetaH)));
}

float jensenSmithGGX(float tan_theta, float m)
{
    float root = m * tan_theta;
    return 2 / (1 + sqrt(1 + root * root));
}

float jensenFresnel(float eta, float cosTheta)
{
    if(cosTheta < 0)
    {
        cosTheta = -cosTheta;
        eta = 1 / eta;
    }

    float sinTheta = sqrt(max(0, 1 - cosTheta * cosTheta));
    float sinThetaT = sinTheta / eta;

    if(sinThetaT >= 1)
        return 1;

    float cosThetaT = sqrt(max(0, 1 - sinThetaT * sinThetaT));
    float para = (eta * cosTheta - cosThetaT)
               / (eta * cosTheta + cosThetaT);
    float perp = (cosTheta - eta * cosThetaT)
               / (cosTheta + eta * cosThetaT);

    return 0.5 * (para * para + perp * perp);
}

float jensenTanTheta(float3 w)
{
    float t = 1 - w.z * w.z;
    return sqrt(max(t, 0)) / w.z;
}

float3 jensenDirectReflection(float3 wi, float3 wo, float eta, float m)
{
    // assert(wi.z >= JENSEN_EPS && wo.z >= JENSEN_EPS);

    float3 wh = normalize(wi + wo);
    float D = jensenDGGX(wh.z, m);
    float F = jensenFresnel(eta, dot(wi, wh));
    float G = jensenSmithGGX(jensenTanTheta(wi), m) *
              jensenSmithGGX(jensenTanTheta(wo), m);

    float v = D * F * G / (4 * wi.z * wo.z);
    return float3(v, v, v);
}

float jensenHG(float cosIO, float g)
{
    float g2 = g * g;
    float dem = 1 + g2 - 2 * g * cosIO;
    return (1 - g2) / (4 * PI * dem * sqrt(dem));
}

float jensenP(float cosIO, float4 HGArg)
{
    // HGArg: wf, gf, wb, gb
    return HGArg.x * jensenHG(cosIO, HGArg.y) +
           HGArg.z * jensenHG(cosIO, HGArg.w);
}

float jensenAccessRhoDt(float3 wi, float eta, float m)
{
    float theta = asin(saturate(abs(wi.z)));
    float u = theta / (0.5 * PI);
    float v = (eta - 1.01) / (10 - 1.01);
    float w = m;
    //return JensenRhoDt.SampleLevel(JensenLinearSampler, float3(u, v, w), 0);
    return JensenRhoDt.SampleLevel(JensenLinearSampler, float3(w, v, u), 0);
}

float3 jensenScatteredReflection(
    float3 wi, float3 wo,
    float eta, float m,
    float3 Rd, float4 HGArg, float alpha, float tauD)
{
    float atti = jensenAccessRhoDt(wi, eta, m);
    float atto = jensenAccessRhoDt(wo, eta, m);

    // single scattered

    /*float singleP = jensenP(dot(wi, wo), HGArg);
    float singleE = exp(- tauD / wi.z - tauD / wo.z);
    float singleDem = wi.z + wo.z;

    float singleScattered = alpha * singleP * (1 - singleE) / singleDem;*/

    // multi scattered

    float3 multiScattered = Rd / PI;

    return atti * (/*singleScattered +*/ multiScattered) * atto;
}

float3 jensenScatteredTransmission(
    float3 wi, float3 wo,
    float frontEta, float backEta, float frontM, float backM,
    float3 Td, float4 HGArg, float alpha, float tauD)
{
    // assert(wi.z * wo.z < 0);

    float atti, atto;
    if(wi.z < 0)
    {
        atti = jensenAccessRhoDt(wi, frontEta, frontM);
        atto = jensenAccessRhoDt(wo, backEta, backM);
    }
    else
    {
        atti = jensenAccessRhoDt(wi, backEta, backM);
        atto = jensenAccessRhoDt(wo, frontEta, frontM);
    }

    float singleScattered;
    float singleP = jensenP(dot(wi, wo), HGArg);
    if(abs(wi.z + wo.z) < JENSEN_EPS)
    {
        float e = exp(-tauD / abs(wo.z));
        singleScattered = abs(alpha * tauD * singleP * e / (wi.z * wo.z));
    }
    else
    {
        float e = exp(-tauD / abs(wi.z)) - exp(-tauD / abs(wo.z));
        singleScattered = abs(alpha * singleP * e / (abs(wi.z) - abs(wo.z)));
    }

    float3 multiScattered = Td / PI;

    return atti * (singleScattered + multiScattered) * atto;
}

struct JensenParams
{
    float etaFront;
    float etaBack;
    float mFront;
    float mBack;
    float3 Rd;
    float3 Td;
    float alpha;
    float tauD;
    float4 HGArg;
};

#define JENSEN_REFL_PDF 0.75

void sampleJensen(
    JensenParams params, float3 wo,
    inout uint rngState, out float3 coef, out float3 wi)
{
    float3 samDir = diffuseSampleZWeightedHemisphere(rngState);

    bool isFront = wo.z < 0;
    bool isReflection = randomFloat(rngState) < JENSEN_REFL_PDF;
    if(isFront == isReflection)
        samDir.z = -samDir.z;

    wi = samDir;

    float fac = PI / (isReflection ? JENSEN_REFL_PDF : (1 - JENSEN_REFL_PDF));

    if(isReflection)
    {
        if(isFront)
        {
            float3 directRefl = jensenDirectReflection(
                -wi, -wo, params.etaFront, params.mFront);
            float3 scatterRefl = jensenScatteredReflection(
                -wi, -wo, params.etaFront, params.mFront,
                params.Rd, params.HGArg, params.alpha, params.tauD);
            coef = fac * (directRefl + scatterRefl);
        }
        else
        {
            float3 directRefl = jensenDirectReflection(
                wi, wo, params.etaBack, params.mBack);
            float3 scatterRefl = jensenScatteredReflection(
                wi, wo, params.etaBack, params.mBack,
                params.Rd, params.HGArg, params.alpha, params.tauD);
            coef = fac * (directRefl + scatterRefl);
        }
    }
    else
    {
        coef = fac * jensenScatteredTransmission(
            wi, wo,
            params.etaFront, params.etaBack, params.mFront, params.mBack,
            params.Td, params.HGArg, params.alpha, params.tauD);
    }
}

#endif // #ifndef PCL_JENSEN

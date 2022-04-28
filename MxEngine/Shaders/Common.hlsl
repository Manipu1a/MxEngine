#define MaxLights 16
#define MIN_REFLECTIVITY 0.04
#define PI 3.1415926
#define PREFILTER_MIP_LEVEL 6
struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // directional/spot light only
    float FalloffEnd; // point/spot light only
    float3 Position; // point light only
    float SpotPower; // spot light only
};


Texture2D MaterialTex[100] : register(t0);
//Texture2D gAlbedoMap : register(t0);
//Texture2D gRoughnessMap : register(t1);
//Texture2D gHeightMap : register(t2);
//Texture2D gNormalMap : register(t3);

//Texture2D gMetallicMap : register(t5);
//Texture2D gAoMap : register(t6);

Texture2D gLutMap : register(t0, space1);
Texture2D gEquirectangularMap : register(t1, space1);
Texture2D gShadowMap : register(t2, space1);
TextureCube gCubeMap : register(t3, space1);
TextureCube gIrradianceMap : register(t4, space1);

TextureCube gPrefilterMap[6] : register(t0, space2);

Texture2D gBuffer0 : register(t0, space3);

//StructuredBuffer<MaterialConstants> gMaterialData : register(t0, space1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    uint gMaterialIndex;
};

// Constant data that varies per material.
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gShadowTransform;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    //float gHeightScale;
    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;
    float Metallic;
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint EmissiveMapIndex;
};

cbuffer cbPrefilter : register(b3)
{
    float roughnessCb;
}


//cbuffer cbMaterial : register(b2)
//{
//    float4 gBaseColor;
//    float4 gDiffuseAlbedo;
//    float3 gFresnelR0;
//    float gSmoothness;
    
//    float4 gAmbientStrength;
//    float4 gSpecularStrength;
//    float gMetallic;
//    float gAo;
//    float4x4 gMatTransform;
//};

struct Surface
{
    float3 position;
    float3 normal;
    float3 interpolatedNormal;
    float3 viewDirection;
    float depth;
    float3 color;
    float alpha;
    float metallic;
    float occlusion;
    float smoothness;
    float roughness;
    float reflectance;
    float fresnelStrength;
    float dither;
};

struct BRDF
{
    float3 diffuse;
    float3 specular;
    float roughness;
    float perceptualRoughness;
    float3 reflectance;
};

//float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
//{
//	// Uncompress each component from [0,1] to [-1,1].
//    float3 normalT = 2.0f * normalMapSample - 1.0f;

//	// Build orthonormal basis.
//    float3 N = unitNormalW;
//    float3 T = normalize(tangentW - dot(tangentW, N) * N);
//    float3 B = cross(N, T);

//    float3x3 TBN = float3x3(T, B, N);

//	// Transform from tangent space to world space.
//    float3 bumpedNormalW = mul(normalT, TBN);

//    return bumpedNormalW;
//}

float OneMinusReflectivity(float metallic)
{
    float range = 1.0 - MIN_REFLECTIVITY;
    return range - metallic * range;
}

//获得0度角反射率
float3 GetF0(float3 albedo, float metallic, float reflectance)
{
    float3 f0 = 0.16f * reflectance * reflectance * (1.0 - metallic) + albedo * metallic;
    return f0;
}

float PerceptualRoughnessToRoughness(float perceptualRoughness)
{
    return perceptualRoughness * perceptualRoughness;
}

float RoughnessToPerceptualSmoothness(float roughness)
{
    return 1.0 - sqrt(roughness);
}
float PerceptualSmoothnessToPerceptualRoughness(float perceptualSmoothness)
{
    return (1.0 - perceptualSmoothness);
}

//float2 ParallaxMapping(float2 texCoords, float3 viewDir)
//{
//    float height = gHeightMap.Sample(gsamLinearWrap, texCoords).r;
//    float2 p = viewDir.xy / viewDir.z * (height * 1.0f);
//    return texCoords - p;
//}

BRDF GetBRDF(Surface surface)
{
    BRDF brdf;
    
    float oneMinusReflectivity = OneMinusReflectivity(surface.metallic);
    brdf.diffuse = surface.color * oneMinusReflectivity;
    brdf.specular = lerp(MIN_REFLECTIVITY, surface.color, surface.metallic);
    
    #if (USE_SMOOTHNESS)
    //brdf.perceptualRoughness = clamp(PerceptualSmoothnessToPerceptualRoughness(surface.smoothness), 0.045f, 1);
    //brdf.roughness = clamp(PerceptualRoughnessToRoughness(brdf.perceptualRoughness), 0.045f, 0.999f);
    #else
    brdf.perceptualRoughness = clamp(surface.roughness, 0.045f, 1);
    brdf.roughness = clamp(surface.roughness * surface.roughness, 0.045f, 0.999f);
    #endif
    
    brdf.reflectance = GetF0(surface.color, surface.metallic, surface.reflectance);
    
    return brdf;
}


//D项
//GGX
float D_GGX(float NoH, float roughness)
{
    float a2 = roughness * roughness;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
}

//优化版GGX
//#define MEDIUMP_FLT_MAX    65504.0
//#define satureMediump(x)   min(x, MEDIUMP_FLT_MAX)

//float D_GGX(float roughness, float NoH, const float3 n, const float3 h)
//{
//    float3 NxH = cross(n, h);
//    float a = NoH * roughness;
//    float k = roughness / (dot(NxH, NxH) + a * a);
//    float d = k * k * (1.0 / PI);
//    return satureMediump(d);
//}

float G_SchlickGGX(float NdotV, float roughness)
{
    float a = roughness; //?
    float k = (a * a) / 2.0;
    
    float nom = NdotV;
    float denom = max(NdotV * (1.0 - k) + k, 0.001f);

    return nom / denom;
}

//G项- Schlick-GGX
float G_Smith(float3 N, float3 V,float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.001f);
    float NdotL = max(dot(N, L), 0.001f);
    float ggx1 = G_SchlickGGX(NdotV, roughness);
    float ggx2 = G_SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

//V项标准版
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5f / (GGXV + GGXL);
    //return GGXV + GGXL;
}

//优化版
float V_SmithGGXCoorelatedFast(float NoV, float NoL, float roughness)
{
    float a = roughness;
    float GGXV = NoL * (NoV * (1.0 - a) + a);
    float GGXL = NoV * (NoL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL);
}

//F-Schlick
float3 F_Schlick(float u, float3 f0, float f90)
{
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

//Schlick，默认90度时反射率是1
float3 F_Schlick(float u, float3 f0)
{
    float f = pow(1.0 - u, 5.0);
    return f0 + f * (1.0 - f0);
}

float3 F_SchlickRoughness(float cosTheta, float3 f0, float roughness)
{
    float3 f = 1.0 - roughness;
    return f0 + (max(f, f0) - f0) * pow(1.0 - cosTheta, 5.0);
}

//Lambert diffuse 最简化的公式
float Fd_Lambert()
{
    return 1.0 / PI;
}

//Renormalized diffuse
float Fd_DisneyRenormalized(float NoV, float NoL, float LoH, float linearRoughness)
{
    float energyBias = lerp(0.0f, 0.5f, linearRoughness);
    
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
    
    float f90 = energyBias + 2.0f * LoH * LoH * linearRoughness;
    
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    
    float lightScatter = F_Schlick(NoL, f0, f90).r;
    float ViewScatter = F_Schlick(NoV, f0, f90).r;

    return lightScatter * ViewScatter * energyFactor;
}

float3 SampleIrradiance(float3 Dir)
{
    float4 environment = gIrradianceMap.Sample(gsamLinearWrap, Dir);

    return environment.rgb;
}

float3 PrefilteredColor(float3 viewDir, float3 normal, float roughness)
{
    float roughnessLevel = roughness * PREFILTER_MIP_LEVEL;
    int fl = floor(roughnessLevel);
    int cl = ceil(roughnessLevel);
    float3 R = reflect(-viewDir, normal);
    float3 flSample = gPrefilterMap[fl].Sample(gsamLinearWrap, R).rgb;
    float3 clSample = gPrefilterMap[cl].Sample(gsamLinearWrap, R).rgb;
    float3 prefilterColor = lerp(flSample, clSample, (roughnessLevel - fl));
    return flSample;
}
float3 DirectBrdf(Surface surface, BRDF brdf, Light light)
{
    float3 L = light.Direction;
    float3 H = normalize(surface.viewDirection + L);
    float3 N = surface.normal;
    float3 V = surface.viewDirection;

    float NoV = (abs(dot(N, V)) + 1e-5);
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float LoH = saturate(dot(L, H));
    float VoH = saturate(dot(V, H));
    
    float G = G_Smith(N, V, L, brdf.perceptualRoughness);
    float G_Vis = (G * VoH) / (NoH * NoV);
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;

    float D = D_GGX(NoH, brdf.roughness);
    float3 f0 = brdf.reflectance;
    float3 F = F_Schlick(VoH, f0);
    float Vis = V_SmithGGXCorrelated(NoV, NoL, brdf.roughness);

    float3 nominator = D * G * F;
    float3 specular = nominator / denominator;
    
   
    //specular BRDF
    float3 Fr = D * F * Vis;
    //diffuse BRDF
    float3 Fd = brdf.diffuse * Fd_DisneyRenormalized(NoV, NoL, LoH, brdf.perceptualRoughness) / PI;
    //float3 Fd = Fd_Lambert();
    //return brdf.roughness * brdf.roughness;
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - surface.metallic;
    
    return (kD * Fd + Fr) * light.Strength * NoL;
}

 //Indirect BRDF
float3 IndirectBRDF(Surface surface, BRDF brdf)
{
    float3 N = surface.normal;
    float3 V = surface.viewDirection;
    float3 R = reflect(-V, N);
    const float MAX_REFLECTION_LOD = 4.0;
    float NoV = clamp((abs(dot(N, V)) + 1e-5), 0.001f, 1.0f);
    
    float3 f0 = brdf.reflectance;
    float3 kS = F_SchlickRoughness(NoV, f0, brdf.roughness);
    float3 kD = 1.0 - kS;
    kD *= 1.0 - surface.metallic;
    
    float3 envBRDF = gLutMap.Sample(gsamLinearWrap, float2(clamp(NoV, 0.001f, 0.999f), brdf.roughness)).rgb;
    
    float3 irradiance = SampleIrradiance(N);
    float3 indirectDiffuse = irradiance * brdf.diffuse;
    
    //IBL-1
    float3 prefilteredColor = PrefilteredColor(V, N, brdf.roughness).rgb;
    //IBL-2
    float3 F = kS;
    
    float3 indirectSpecular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    
    float3 ambient = (kD * indirectDiffuse + indirectSpecular) * surface.occlusion;
    
    return float4(ambient, 1.0);
    
}
//---------------------------------------------------------------------------------------
// PCF for shadow mapping.
//---------------------------------------------------------------------------------------

float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}
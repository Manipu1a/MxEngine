//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
#include "Common.hlsl"


struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 Tangent : TANGENT;
};


struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float4 ShadowPosH : POSITION1;
    float3 PosL : POSITION2;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    float3 Tangent : TANGENT0;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space.
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, MatTransform).xy;
    //vout.TexC = texC.xy;
    vout.PosL = vin.PosL;
    //float H = gHeightMap.SampleLevel(gsamLinearWrap, vin.TexC, 0.0).x * 1.0f;
    //float3 posL = vin.PosL + vin.NormalL * H;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    float3 T = normalize(vin.Tangent);
    
    //H = 2 * H - 1.0;
    //vout.Tangent = T;
    vout.Tangent = mul(T, (float3x3) gWorld);
    //vout.PosW += vout.NormalW * H;
    
    // Transform to homogeneous clip space.
    vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
    vout.ShadowPosH = mul(posW, gShadowTransform);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    Surface surface;
    surface.position = pin.PosW;
    
    float3 N = normalize(pin.NormalW);
    float3 B = cross(N, pin.Tangent);
    float3x3 TBN = float3x3(pin.Tangent, B, N);
    
    //float3 TangentViewPos = mul(gEyePosW, TBN);
    //float3 TangentFragPos = mul(pin.PosW, TBN);
    //float3 ViewDir = normalize(TangentViewPos - TangentFragPos);
    //float2 texCoords = ParallaxMapping(pin.TexC, ViewDir);
    //pin.TexC = texCoords;
    
    if (NormalMapIndex != -1)
    {
         // 从法线贴图范围[0,1]获取法线
        float3 normal =  MaterialTex[NormalMapIndex].Sample(gsamLinearWrap, pin.TexC).rgb;
        // 将法线向量转换为范围[-1,1]
        surface.normal = mul(normalize(normal * 2.0 - 1.0), TBN);
    }
    else
    {
        surface.normal = pin.NormalW;

    }
   // surface.color = MaterialTex[AlbedoMapIndex].Sample(gsamLinearWrap, pin.TexC).rgb;
    surface.viewDirection = normalize(gEyePosW - pin.PosW);
    
    if (AlbedoMapIndex != -1)
    {
        surface.color = MaterialTex[AlbedoMapIndex].Sample(gsamLinearWrap, pin.TexC).rgb;
    }
    else
    {
        surface.color = DiffuseAlbedo.rgb;
    }
    
    surface.alpha = DiffuseAlbedo.a;
    surface.metallic = Metallic;
    
    if (RoughnessMapIndex != -1)
    {
        surface.roughness = MaterialTex[RoughnessMapIndex].Sample(gsamLinearWrap, pin.TexC).r;
    }
    else
    {
        surface.roughness = Roughness;
    }
    
    surface.reflectance = 0.5f;
    surface.occlusion = 1.0f;
    
    BRDF brdf = GetBRDF(surface);
    
    float shadowFactor = 1.0f;
    
    shadowFactor = CalcShadowFactor(pin.ShadowPosH);
    
    float3 color = IndirectBRDF(surface, brdf);
    
    for (int i = 0; i < 1; ++i)
    {
        color += DirectBrdf(surface, brdf, gLights[i]) * shadowFactor;
    }
    
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    
    return float4(color, 1.0f);
}



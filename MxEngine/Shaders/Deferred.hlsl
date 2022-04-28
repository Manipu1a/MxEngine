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

struct MRT
{
    float4 OutColor0 : COLOR0;
    float4 OutColor1 : COLOR1;
    float4 OutColor2 : COLOR2;
    float4 OutColor3 : COLOR3;
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

MRT PS(VertexOut pin) : SV_Target
{
    MRT mrt;
    mrt.OutColor0 = float4(1.0, 0.0, 0.0, 0.0);
    mrt.OutColor1 = float4(0.0, 1.0, 0.0, 0.0);
    mrt.OutColor2 = float4(0.0, 0.0, 1.0, 0.0);
    mrt.OutColor3 = float4(1.0, 1.0, 1.0, 0.0);
    
    return mrt;
}



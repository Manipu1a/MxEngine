//***************************************************************************************
// ShadowDebug.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Include common HLSL code.
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
    float3 PosL : POSITION0;
    float3 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space.

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.PosL = vin.PosL;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    //return float4(pin.NormalW, 1.0);
    
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}



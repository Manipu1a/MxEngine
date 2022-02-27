//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//=============================================================================

// Include common HLSL code.
#include "Common.hlsl"

static const float2 invAtan = float2(0.1591, 0.3183);

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.x, v.z),asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	uv.y = 1.0 - uv.y;
	return uv;
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Use local vertex position as cubemap lookup vector.
	vout.PosL = vin.PosL;
	
	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// Always center sky about camera.
	posW.xyz += gEyePosW;

	//vout.TexC = vin.TexC;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vout.PosH = mul(posW, gViewProj).xyww;
	
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	//return float4(1.0, 1.0, 1.0, 1.0);

	float2 uv = SampleSphericalMap(normalize(pin.PosL));

	float3 color = gEquirectangularMap.Sample(gsamLinearWrap, uv).xyz;
	
    return float4(color, 1.0);
	//
	//if (color.x > 0)
	//{
	//	return float4(1.0,1.0,1.0,1.0);
	//}
	//else
	//{
	//	return float4(1.0,0.0,0.0, 1.0);
	//}

	//return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
	
	
}


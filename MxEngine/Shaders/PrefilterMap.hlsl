//***************************************************************************************
// ShadowDebug.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
	float3 PosL	   : POSITION;
};

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
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
    
    float3 prefilteredColor = (float3) 0.0f;
    float3 R = normalize(pin.PosL);
    float LinearRoughness = roughnessCb;
    
    //假设N V R都是一个方向
    float3 N = R;
    float3 V = R;
    
    const uint SAMPLE_COUNT = 1024u;
    const float SPEC_TEX_WIDTH = 512;
                
    float totalWeight = 0.0;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 xi = Hammersley(i, SAMPLE_COUNT);
        float3 halfway = ImportanceSampleGGX(xi, N, LinearRoughness);
        //反向推导光线方向
        float3 lightVec = normalize(2.0 * dot(V, halfway) * halfway - V);
                    
        float NdotL = max(dot(N, lightVec), 0.0);
        float NdotH = saturate(dot(N, halfway));
        float HdotV = saturate(dot(halfway, V));
                    
        if (NdotL > 0.0)
        {
            //根据pdf来计算mipmap等级
            float D = D_GGX(NdotH, LinearRoughness);
            float pdf = (D * NdotH / (4 * HdotV) + 0.0001f);

            float saTexel = 4.0f * PI / (6.0f * SPEC_TEX_WIDTH * SPEC_TEX_WIDTH);
            float saSample = 1.0f / (SAMPLE_COUNT * pdf + 0.00001f);

            float mipLevel = LinearRoughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);
            
            prefilteredColor += gCubeMap.SampleLevel(gsamLinearWrap, lightVec, mipLevel).rgb * NdotL;
            //prefilteredColor += SAMPLE_TEXTURECUBE(envMap, samplerEnv, lightVec) * NdotL;
            totalWeight += NdotL;
        }
    }
    //dw
    prefilteredColor = prefilteredColor / totalWeight;
    
    return float4(prefilteredColor, 1.0f);
}



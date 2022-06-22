#pragma once

#include "MxRendering/Common/d3dUtil.h"
#include "MxRendering/Common/MathHelper.h"
#include "UploadBuffer.h"

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT     MaterialIndex;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

struct PrefilterConstants
{
	float Roughness = 0.0f;
};

struct Vertex
{
	Vertex() {}

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT2 tex, DirectX::XMFLOAT3 tangent, DirectX::XMFLOAT3 binor)
		:Position(pos), Normal(normal), TexCoords(tex), Tangent(tangent), Binormal(binor) {}

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT2 tex)
		:Position(pos), Normal(normal), TexCoords(tex) {}

	Vertex(float xpos, float ypos, float zpos, float xnormal, float ynormal, float znormal, float xtex, float ytex)
	{

	}
	//坐标
	DirectX::XMFLOAT3 Position;
	//法线
	DirectX::XMFLOAT3 Normal;
	//纹理坐标
	DirectX::XMFLOAT2 TexCoords;

	//切线
	DirectX::XMFLOAT3 Tangent;
	//
	DirectX::XMFLOAT3 Binormal;
};

struct TempVertex
{
	//坐标
	DirectX::XMFLOAT3 Position;
};
// Stores the resources needed for the CPU to build the command lists
// for a frame.  
namespace MxRendering::Resources
{
	struct FrameResource
	{
	public:

		FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
		FrameResource(const FrameResource& rhs) = delete;
		FrameResource& operator=(const FrameResource& rhs) = delete;
		~FrameResource();

		// We cannot reset the allocator until the GPU is done processing the commands.
		// So each frame needs their own allocator.
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

		// We cannot update a cbuffer until the GPU is done processing the commands
		// that reference it.  So each frame needs their own cbuffers.
		std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
		std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
		std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;

		// Fence value to mark commands up to this fence point.  This lets us
		// check if these frame resources are still in use by the GPU.
		UINT64 Fence = 0;
	};
}

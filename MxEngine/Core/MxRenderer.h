#pragma once
//***************************************************************************************
// 
// Render Module
// 
//***************************************************************************************

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/MxRes.h"
#include "../Common/FrameResource.h"
#include "../Common/Camera.h"
#include "../Common/EngineConfig.h"
#include "../Common/ShadowMap.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

class MxRenderer : public D3DApp 
{
public:
	MxRenderer(HINSTANCE hInstance);
	MxRenderer(const MxRenderer& rhs) = delete;
	MxRenderer& operator=(const MxRenderer& rhs) = delete;
	~MxRenderer();

	virtual bool Initialize() override;
private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
	virtual void CreateRtvAndDsvDescriptorHeaps()override;

	void OnKeyboardInput(const GameTimer& gt);
	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	void UpdateCamera(const GameTimer& gt);
	//void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateShadowTransform(const GameTimer& gt);
	void UpdateShadowPassCB(const GameTimer& gt);

	//Build pipeline
	//void CreateDSRTandHeap();
	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuileSourceBufferViews();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawSceneToShadowMap();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
	//load res
	void LoadTexture();
	void BuildMaterial();
private:
	//frame
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;
	//heap
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mGlobalSrvDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mMaterialSrvDescriptorHeap = nullptr;
	//
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, std::shared_ptr<Material>> mMaterials;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	PassConstants mMainPassCB; // index 0 of pass cbuffer.
	PassConstants mShadowPassCB; // index 1 of pass cbuffer.

	UINT mPassCbvOffset = 0;
	bool mIsWireframe = false;

	UINT mMatCbvOffset = 0;
	UINT mTexSrvOffset = 0;
	UINT mSkyTexSrvOffset = 0;
	UINT mShadowMapSrvOffset = 0;
	Camera mCamera;
	Camera mLightCamera;

	//shadow
	std::unique_ptr<ShadowMap> mShadowMap;
	DirectX::BoundingSphere mSceneBounds;
	//LightData
	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mDirectLightDir;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();


	//D3D12_CPU_DESCRIPTOR_HANDLE RTV_Handles[GBUFFER_COUNT];
	XMFLOAT3 mEyePos = { 0.0f, 100.0f, 0.0f };
	XMFLOAT3 inputPos = { 0.0f, 0.0f, 0.0f };
	XMVECTOR LookDir = { 0.0f, 0.0f, 1.0f,0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;


};
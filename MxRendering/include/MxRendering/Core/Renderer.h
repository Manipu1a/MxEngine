#pragma once
//***************************************************************************************
// 
// Render Module
// 
//***************************************************************************************

#include "MxRendering/Common/d3dUtil.h"
#include "MxRendering/Common/GameTimer.h"
#include "MxRendering/Common/MathHelper.h"
#include "MxRendering/Resources/UploadBuffer.h"
#include "MxRendering/Resources/MxRes.h"
#include "MxRendering/Resources/FrameResource.h"
#include "MxRendering/Common/Camera.h"
#include "MxRendering/Common/EngineConfig.h"
#include "MxRendering/Resources/ShadowMap.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")


namespace MxRendering::Core
{
	class MxGui;
	class MxRenderTarget;
	class MxCubeRenderTarget;


	using Microsoft::WRL::ComPtr;
	using namespace DirectX;
	using namespace DirectX::PackedVector;

	class MxRenderer
	{
	public:
		MxRenderer();
		MxRenderer(const MxRenderer& rhs) = delete;
		MxRenderer& operator=(const MxRenderer& rhs) = delete;
		~MxRenderer();

		bool Initialize(HINSTANCE* hInstance, HWND* p_Hwnd, ID3D12Device* p_Device);
		void Tick(const GameTimer& gt);


		HINSTANCE AppInst()const;
		HWND      MainWnd()const;
		float     AspectRatio()const;
		static MxRenderer* GetRenderer();
		ComPtr<ID3D12Device> GetDevice() { return md3dDevice; }
		ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return mCommandList; }

		//存储资源
		void SaveMesh(std::unique_ptr<MeshGeometry>& mgo);
		void SaveTexturePath(const std::string& name, std::wstring& path);

	private:
		void OnResize();
		void Update(const GameTimer& gt);
		void Draw(const GameTimer& gt);
		void CreateRtvAndDsvDescriptorHeaps();
		//initialize d3d hardware
		bool InitDirect3D();
		void CreateCommandObjects();
		void CreateSwapChain();
		void FlushCommandQueue();

		ID3D12Resource* CurrentBackBuffer()const;
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;
		void LogAdapters();
		void LogAdapterOutputs(IDXGIAdapter* adapter);
		void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

		void OnKeyboardInput(const GameTimer& gt);
		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp(WPARAM btnState, int x, int y);
		void OnMouseMove(WPARAM btnState, int x, int y);
		void UpdateCamera(const GameTimer& gt);
		//void AnimateMaterials(const GameTimer& gt);
		void UpdateObjectCBs(const GameTimer& gt);
		void UpdateMaterialCBs(const GameTimer& gt);
		void UpdateMainPassCB(const GameTimer& gt);
		void UpdateShadowTransform(const GameTimer& gt);
		void UpdateShadowPassCB(const GameTimer& gt);

		void BuildPrefilterPassCB();

		//更新cubemap变换矩阵数据
		void UpdateCubeMapFacePassCBs();
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
		void BuildCubeFaceCamera(float x, float y, float z);
		void BuildCubeDepthStencil();

		void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
		//绘制depth map
		void DrawSceneToShadowMap();
		//绘制cube map
		void DrawSceneToCubeMap();
		void DrawIrradianceCubeMap();
		void DrawPrefilterCubeMap();
		//gbuffer pass
		void DrawGBufferMap();

		std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
		//load res
		void LoadTexture();
		void BuildMaterial();
		void LoadModel();

	protected:
		static MxRenderer* mRenderer;

	private:

		ComPtr<HINSTANCE> mhAppInst = nullptr;
		ComPtr<HWND> mhMainWnd = nullptr;
		//HINSTANCE mhAppInst = nullptr; // application instance handle
		//HWND      mhMainWnd = nullptr; // main window handle
		ComPtr<ID3D12Device> md3dDevice;
		ComPtr<IDXGIFactory4> mdxgiFactory;
		ComPtr<IDXGISwapChain> mSwapChain;
		ComPtr<ID3D12Fence> mFence;
		UINT64 mCurrentFence = 0;

		ComPtr<ID3D12CommandQueue> mCommandQueue;
		ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
		ComPtr<ID3D12GraphicsCommandList> mCommandList;

		static const int SwapChainBufferCount = 2;
		int mCurrBackBuffer = 0;
		ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
		ComPtr<ID3D12Resource> mDepthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> mRtvHeap;
		ComPtr<ID3D12DescriptorHeap> mDsvHeap;

		D3D12_VIEWPORT mScreenViewport;
		D3D12_RECT mScissorRect;

		UINT mRtvDescriptorSize = 0;
		UINT mDsvDescriptorSize = 0;
		UINT mCbvSrvUavDescriptorSize = 0;

		// Derived class should set these in derived constructor to customize starting values.
		std::wstring mMainWndCaption = L"d3d App";
		D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
		DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		DXGI_FORMAT mRtvFormat[3] = { DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM };
		int mClientWidth = 1280;
		int mClientHeight = 800;

		// Set true to use 4X MSAA (?.1.8).  The default is false.
		bool      m4xMsaaState = false;    // 4X MSAA enabled
		UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

		//
		std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
		std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
		std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
		std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
		std::unordered_map<std::string, std::shared_ptr<Material>> mMaterials;
		//tex
		std::unordered_map<std::string, std::wstring> MaterialTex;

		//frame
		std::vector<std::unique_ptr<MxRendering::Resources::FrameResource>> mFrameResources;
		MxRendering::Resources::FrameResource* mCurrFrameResource = nullptr;
		int mCurrFrameResourceIndex = 0;
		//heap
		ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
		ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

		ComPtr<ID3D12DescriptorHeap> mGlobalSrvDescriptorHeap = nullptr;
		ComPtr<ID3D12DescriptorHeap> mMaterialSrvDescriptorHeap = nullptr;

		//

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
		UINT mEnvCubeMapSrvOffset = 0;
		UINT mIrradianceMapSrvOffset = 0;
		UINT mPrefilterMapSrvOffset = 0;
		UINT mGBufferMapSrvOffset = 0;

		Camera mCamera;
		Camera mLightCamera;
		Camera mCubeMapCamera[6];

		//shadow
		std::unique_ptr<MxRendering::Resources::ShadowMap> mShadowMap;
		DirectX::BoundingSphere mSceneBounds;

		//MRT
		std::unique_ptr<MxRenderTarget> mGBufferMRT;

		//LightData
		float mLightNearZ = 0.0f;
		float mLightFarZ = 0.0f;
		XMFLOAT3 mDirectLightDir;

		float DirectLightX = 0.0f;
		float DirectLightY = 0.57735f;
		float DirectLightZ = -0.57735f;

		XMFLOAT3 mLightPosW;
		XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
		XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
		XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

		XMMATRIX captureProjection;
		XMMATRIX captureViews[6];

		std::unique_ptr<MxCubeRenderTarget> mEnvCubeMap = nullptr;
		std::unique_ptr<MxCubeRenderTarget> mIrradianceCubeMap = nullptr;
		std::map<int, std::unique_ptr<MxCubeRenderTarget>> mPrefilterCubeMap;
		std::map<int, ComPtr<ID3D12Resource>> mPrefilterDepthStencilBuffers;
		std::unique_ptr<UploadBuffer<PrefilterConstants>> PrefilterCB = nullptr;

		CD3DX12_CPU_DESCRIPTOR_HANDLE mCubeDSV;
		std::map<int, CD3DX12_CPU_DESCRIPTOR_HANDLE > mPrefilterCubeDSVs;

		ComPtr<ID3D12Resource> mCubeDepthStencilBuffer;

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

		bool PrePass = false;
	};
}
#include "MxRenderer.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/LoadTexture/Resource.h"
#include "../Common/MxCubeRenderTarget.h"
#include "../Common/MxRenderTarget.h"
#include "../Common/MxGui.h"
#include "MxWorld.h"
#include "MxLevel.h"
#include "MxRenderComponent.h"

//#define CGLTF_IMPLEMENTATION
const int gNumFrameResources = 3;
const int gPrefilterLevel = 6;
const UINT CubeMapSize = 512;

namespace MxEngine
{
	MxRenderer* MxRenderer::mRenderer = nullptr;
	MxRenderer* MxRenderer::GetRenderer()
	{
		return mRenderer;
	}
}

MxEngine::MxRenderer::MxRenderer()
{
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = 60.0;
	mRenderer = this;
}

MxEngine::MxRenderer::~MxRenderer()
{
	if(md3dDevice != nullptr)
		FlushCommandQueue();

	Gui->clear();
}


bool MxEngine::MxRenderer::Initialize(HINSTANCE hInstance, HWND hWnd)
{
	mhAppInst = hInstance;
	mhMainWnd = hWnd;
	
	if(!InitDirect3D())
		return false;

	// Do the initial resize code.
	OnResize();

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
	BuildCubeFaceCamera(0.0f, 0.0f, 0.0f);
	mShadowMap = std::make_unique<ShadowMap>(
		md3dDevice.Get(), 2048, 2048);

	mEnvCubeMap = std::make_unique<MxCubeRenderTarget>(md3dDevice.Get(),
		CubeMapSize, CubeMapSize, DXGI_FORMAT_R8G8B8A8_UNORM);

	mIrradianceCubeMap = std::make_unique<MxCubeRenderTarget>(md3dDevice.Get(),
		CubeMapSize, CubeMapSize, DXGI_FORMAT_R8G8B8A8_UNORM);

	PrefilterCB = std::make_unique<UploadBuffer<PrefilterConstants>>(md3dDevice.Get(), gPrefilterLevel, true);

	for (int i = 0; i < gPrefilterLevel; ++i)
	{
		mPrefilterCubeMap[i] = std::make_unique<MxCubeRenderTarget>(md3dDevice.Get(),
			(UINT)(CubeMapSize / std::pow(2 , i)), (UINT)(CubeMapSize / std::pow(2, i)), DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	mGBufferMRT = std::make_unique<MxRenderTarget>(md3dDevice.Get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM,1,4);

	Gui = std::make_unique<MxGui>();

	LoadModel();
	LoadTexture();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildMaterial();
	BuildRenderItems();
	BuildCubeDepthStencil();
	BuildFrameResources();
	BuildConstantBufferViews();
	BuileSourceBufferViews();
	BuildPSOs();

	BuildPrefilterPassCB();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void MxEngine::MxRenderer::SaveMesh(std::unique_ptr<MeshGeometry>& mgo)
{
	mGeometries[mgo->Name] = std::move(mgo);
}

void MxEngine::MxRenderer::SaveTexturePath(const std::string& name, std::wstring& path)
{
	MaterialTex.insert({ name, path });
}

void MxEngine::MxRenderer::Tick(const GameTimer& gt)
{
	Update(gt);
	Draw(gt);
}

void MxEngine::MxRenderer::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();
	
	// Resize the swap chain.
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mClientWidth, mClientHeight, 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    // Create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = mDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

    // Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	
    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	mLightCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void MxEngine::MxRenderer::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	//UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	//更新平行光方向
	mDirectLightDir = XMFLOAT3(DirectLightX, DirectLightY, DirectLightZ);

	UpdateObjectCBs(gt);
	UpdateMainPassCB(gt);
	UpdateMaterialCBs(gt);
	UpdateShadowTransform(gt);
	UpdateShadowPassCB(gt);
}

void MxEngine::MxRenderer::Draw(const GameTimer& gt)
{
	//mCommandList->Close();
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	{
		ID3D12DescriptorHeap* descriptorHeaps[] = { mMaterialSrvDescriptorHeap.Get() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		//绑定根签名
		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

		//skymap
		CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mMaterialSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		skyTexDescriptor.Offset(mSkyTexSrvOffset, mCbvSrvUavDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(4, skyTexDescriptor);

		//绑定所有贴图
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mMaterialSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		mCommandList->SetGraphicsRootDescriptorTable(3, tex);
	}

	if (!PrePass)
	{
		PrePass = true;

		DrawSceneToCubeMap();
		DrawIrradianceCubeMap();

		DrawPrefilterCubeMap();
	}

	DrawSceneToShadowMap();

	DrawGBufferMap();
	CD3DX12_GPU_DESCRIPTOR_HANDLE prefilterTexDescriptor = mPrefilterCubeMap[0]->Srv();
	//prefilterTexDescriptor.Offset(mPrefilterMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(5, prefilterTexDescriptor);

	//pass数据以contant形式传入
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	CD3DX12_GPU_DESCRIPTOR_HANDLE gBufferDescriptor(mMaterialSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	gBufferDescriptor.Offset(mGBufferMapSrvOffset, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(7, gBufferDescriptor);

	mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
	// 
	mCommandList->SetPipelineState(mPSOs["debug"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Debug]);

	mCommandList->SetPipelineState(mPSOs["sky"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Sky]);

	Gui->tick_pre();

	Gui->draw_frame();

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void MxEngine::MxRenderer::CreateRtvAndDsvDescriptorHeaps()
{
	// Add +6 RTV for cube render target.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + /*12 + 6 * gPrefilterLevel*/ + 1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// Add +1 DSV for shadow map.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 3;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	mCubeDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
		2,
		mDsvDescriptorSize);

	//for (int i = 0; i < gPrefilterLevel; ++i)
	//{
	//	mPrefilterCubeDSVs[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 3 + i, mDsvDescriptorSize);
	//}

}

bool MxEngine::MxRenderer::InitDirect3D()
{
	#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Check 4X MSAA quality support for our back buffer format.
    // All Direct3D 11 capable devices support 4X MSAA for all render 
    // target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
	
#ifdef _DEBUG
    LogAdapters();
#endif

	CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void MxEngine::MxRenderer::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();
}

void MxEngine::MxRenderer::CreateSwapChain()
{
	// Release the previous swapchain we will be recreating.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd, 
		mSwapChain.GetAddressOf()));
}

void MxEngine::MxRenderer::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
	mCurrentFence++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if(mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* MxEngine::MxRenderer::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE MxEngine::MxRenderer::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE MxEngine::MxRenderer::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void MxEngine::MxRenderer::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);
        
		++i;
	}

	for(size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void MxEngine::MxRenderer::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);
        
		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void MxEngine::MxRenderer::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for(auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

void MxEngine::MxRenderer::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera.Elevated(-10.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera.Elevated(10.0f * dt);

	mCamera.UpdateViewMatrix();
}

void MxEngine::MxRenderer::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void MxEngine::MxRenderer::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MxEngine::MxRenderer::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void MxEngine::MxRenderer::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta) + inputPos.x;
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta) + inputPos.z;
	mEyePos.y = mRadius * cosf(mPhi);

	//mEyePos.x = inputPos.x;
	//mEyePos.z = inputPos.z;
	//mEyePos.y = 0.0f;

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	//XMVECTOR target = XMVectorZero();
	XMVECTOR target = pos + LookDir * 500.0f;
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	std::cout << mEyePos.x << std::endl;

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void MxEngine::MxRenderer::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();

	for (auto& e : mAllRitems) {
		if (e->NumFramesDirty > 0) {
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			objConstants.MaterialIndex = e->Mat->MatCBIndex;

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void MxEngine::MxRenderer::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			matConstants.Metallic = mat->Metallic;
			matConstants.AlbedoMapIndex = mat->AlbedoSrvHeapIndex;
			matConstants.NormalMapIndex = mat->NormalSrvHeapIndex;
			matConstants.RoughnessMapIndex = mat->RoughnessSrvHeapIndex;
			matConstants.EmissiveMapIndex = mat->EmissiveSrvHeapIndex;
			
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));
			//matConstants.AmbientStrength = mat->AmbientColor;
			//matConstants.SpecularStrength = mat->SpecularStrength;
			//matConstants.Metallic = mat->Metallic;
			//matConstants.Ao = mat->Ao;

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void MxEngine::MxRenderer::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();
	//XMMATRIX view = XMLoadFloat4x4(&mLightView);
	//XMMATRIX proj = XMLoadFloat4x4(&mLightProj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);
	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));

	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.0f,0.0f,0.0f, 1.0f };
	mMainPassCB.Lights[0].Direction = mDirectLightDir;
	mMainPassCB.Lights[0].Strength = { 5.0f, 5.0f, 5.0f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);

	UpdateCubeMapFacePassCBs();
}

void MxEngine::MxRenderer::UpdateShadowTransform(const GameTimer& gt)
{
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&mDirectLightDir);
	XMVECTOR lightPos = 2.0f * mSceneBounds.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	mLightCamera.SetPosition(lightPos.m128_f32[0], lightPos.m128_f32[1], lightPos.m128_f32[2]);
	mLightCamera.UpdateViewMatrix();
	XMStoreFloat3(&mLightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - mSceneBounds.Radius;
	float b = sphereCenterLS.y - mSceneBounds.Radius;
	float n = sphereCenterLS.z - mSceneBounds.Radius;
	float r = sphereCenterLS.x + mSceneBounds.Radius;
	float t = sphereCenterLS.y + mSceneBounds.Radius;
	float f = sphereCenterLS.z + mSceneBounds.Radius;

	mLightNearZ = n;
	mLightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);
	XMStoreFloat4x4(&mShadowTransform, S);
}

void MxEngine::MxRenderer::UpdateShadowPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	UINT w = mShadowMap->Width();
	UINT h = mShadowMap->Height();

	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mShadowPassCB.EyePosW = mLightPosW;
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = mLightNearZ;
	mShadowPassCB.FarZ = mLightFarZ;

	auto currPassCB = mCurrFrameResource->PassCB.get();
	//阴影pass数据offset = 1
	currPassCB->CopyData(1, mShadowPassCB);
}

void MxEngine::MxRenderer::BuildPrefilterPassCB()
{
	for (int i = 0; i < gPrefilterLevel; ++i)
	{
		PrefilterConstants Constants;
		Constants.Roughness = (float)i / (float)(gPrefilterLevel - 1);
		PrefilterCB->CopyData(i, Constants);
	}
}

void MxEngine::MxRenderer::UpdateCubeMapFacePassCBs()
{
	for (int i = 0; i < 6; ++i)
	{
		PassConstants cubeFacePassCB = mMainPassCB;

		//XMMATRIX view = captureViews[i];
		//XMMATRIX proj = captureProjection;
		XMMATRIX view = mCubeMapCamera[i].GetView();
		XMMATRIX proj = mCubeMapCamera[i].GetProj();
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
		XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
		XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
		XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

		XMStoreFloat4x4(&cubeFacePassCB.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cubeFacePassCB.InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&cubeFacePassCB.Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&cubeFacePassCB.InvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&cubeFacePassCB.ViewProj, XMMatrixTranspose(viewProj));
		XMStoreFloat4x4(&cubeFacePassCB.InvViewProj, XMMatrixTranspose(invViewProj));
		XMStoreFloat4x4(&cubeFacePassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));

		cubeFacePassCB.EyePosW = XMFLOAT3(0.0f, 0.0f, 0.0f);
		cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)CubeMapSize, (float)CubeMapSize);
		cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / CubeMapSize, 1.0f / CubeMapSize);

		auto currPassCB = mCurrFrameResource->PassCB.get();

		// Cube map pass cbuffers are stored in elements 1-6.
		currPassCB->CopyData(2 + i, cubeFacePassCB);
	}
}

void MxEngine::MxRenderer::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)mAllRitems.size();
	UINT matCount = (UINT)mMaterials.size();
	UINT texCount = (UINT)mTextures.size();

	//UINT texCount = (UINT)GImportFBX->mSRVMap.size();
	// Need a CBV descriptor for each object for each frame resource,
	// +1 for the perPass CBV for each frame resource.
	// +texCount  for the tex mat for each object
	// + 1 for the Equirectangular map
	// + 1 for the irradiance map
	// + 1 for the prefilter map
	// + 1 for the nulltex map
	// + 1 for the shadowmaps

	UINT numDescriptors = texCount
		+1 //shadowmap
		+1 //EnvCubeMap
		+1 //IrradianceMap
		+gPrefilterLevel //all level for prefilter
		+ 4 //gbuffer
		;

	//each frame
	//0 ~ objcount
	//objcount ~ objcount + matcount

	// Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
	mShadowMapSrvOffset = texCount;
	mEnvCubeMapSrvOffset = mShadowMapSrvOffset + 1;
	mIrradianceMapSrvOffset = mEnvCubeMapSrvOffset + 1;
	mPrefilterMapSrvOffset = mIrradianceMapSrvOffset + 1;
	mGBufferMapSrvOffset = mPrefilterMapSrvOffset + 6;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mGlobalSrvDescriptorHeap)));


	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.NumDescriptors = numDescriptors;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc,
		IID_PPV_ARGS(&mMaterialSrvDescriptorHeap)));


	Gui->Initialize(md3dDevice.Get(), mhMainWnd);
	//D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	//desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//desc.NumDescriptors = 1;
	//desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//desc.NodeMask = 0;
	//if (md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mGuiSrvDescriptorHeap)) != S_OK)
	//	return;

	////初始化GUI
	//// 
	//IMGUI_CHECKVERSION();
	//ImGui::CreateContext();
	//ImGuiIO& io = ImGui::GetIO(); (void)io;

	//// Setup Dear ImGui style
	//ImGui::StyleColorsDark();

	//// Setup Platform/Renderer backends
	//ImGui_ImplWin32_Init(mhMainWnd);
	//ImGui_ImplDX12_Init(md3dDevice.Get(), gNumFrameResources,
	//	DXGI_FORMAT_R8G8B8A8_UNORM, mGuiSrvDescriptorHeap.Get(),
	//	mGuiSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
	//	mGuiSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

}

void MxEngine::MxRenderer::BuildConstantBufferViews()
{

}

void MxEngine::MxRenderer::BuileSourceBufferViews()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	for (auto& res : mTextures)
	{
		if (res.second->Resource != nullptr)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mMaterialSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			hDescriptor.Offset(res.second->TexHeapIndex, mCbvSrvUavDescriptorSize);

			srvDesc.Format = res.second->Resource->GetDesc().Format;
			md3dDevice->CreateShaderResourceView(res.second->Resource.Get(), &srvDesc, hDescriptor);
		}
	}

	auto srvCpuStart = mMaterialSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mMaterialSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	auto rtvCpuStart = mRtvHeap->GetCPUDescriptorHandleForHeapStart();

	mShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapSrvOffset, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapSrvOffset, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));

	int rtvOffset = SwapChainBufferCount;
	int irradianceOffset = rtvOffset + 6;

	//CD3DX12_CPU_DESCRIPTOR_HANDLE cubeRtvHandles[6];
	//for (int i = 0; i < 6; ++i)
	//	cubeRtvHandles[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, rtvOffset + i, mRtvDescriptorSize);

	//mEnvCubeMap->BuildDescriptors(
	//	CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mEnvCubeMapSrvOffset, mCbvSrvUavDescriptorSize),
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mEnvCubeMapSrvOffset, mCbvSrvUavDescriptorSize),
	//	cubeRtvHandles
	//);

	mEnvCubeMap->BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mEnvCubeMapSrvOffset, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mEnvCubeMapSrvOffset, mCbvSrvUavDescriptorSize));

	//CD3DX12_CPU_DESCRIPTOR_HANDLE irradiancecubeRtvHandles[6];
	//for (int i = 0; i < 6; ++i)
	//	irradiancecubeRtvHandles[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, irradianceOffset + i, mRtvDescriptorSize);

	//mIrradianceCubeMap->BuildDescriptors(
	//	CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mIrradianceMapSrvOffset, mCbvSrvUavDescriptorSize),
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mIrradianceMapSrvOffset, mCbvSrvUavDescriptorSize),
	//	irradiancecubeRtvHandles
	//);

	mIrradianceCubeMap->BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mIrradianceMapSrvOffset, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mIrradianceMapSrvOffset, mCbvSrvUavDescriptorSize));

	int prefilterRtvOffset = irradianceOffset + 6;
	for (int i = 0; i < gPrefilterLevel; ++i)
	{
		//CD3DX12_CPU_DESCRIPTOR_HANDLE prefilterRtvHandles[6];
		//for (int j = 0; j < 6; ++j)
		//{
		//	prefilterRtvHandles[j] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, prefilterRtvOffset + j, mRtvDescriptorSize);
		//}

		//mPrefilterCubeMap[i]->BuildDescriptors(
		//	CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mPrefilterMapSrvOffset, mCbvSrvUavDescriptorSize),
		//	CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mPrefilterMapSrvOffset, mCbvSrvUavDescriptorSize),
		//	prefilterRtvHandles
		//);
		mPrefilterCubeMap[i]->BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mPrefilterMapSrvOffset, mCbvSrvUavDescriptorSize),
			CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mPrefilterMapSrvOffset, mCbvSrvUavDescriptorSize));

		prefilterRtvOffset += 6;
		mPrefilterMapSrvOffset++; //offset
	}

	int gbufferRtvOffset = prefilterRtvOffset;
	//build mrt

	mGBufferMRT->BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mGBufferMapSrvOffset, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mGBufferMapSrvOffset, mCbvSrvUavDescriptorSize));

	//mRenderTarget0->BuildDescriptors(
	//	CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapSrvOffset, mCbvSrvUavDescriptorSize),
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapSrvOffset, mCbvSrvUavDescriptorSize),
	//	CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, gbufferRtvOffset, mRtvDescriptorSize));

}

void MxEngine::MxRenderer::BuildRootSignature()
{
	//创建描述符表
	CD3DX12_DESCRIPTOR_RANGE texTable[4];
	texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 100, 0); //bindless space0
	texTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 1); //t0 space1
	texTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, gPrefilterLevel, 0, 2); //t0 space2
	texTable[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 3); //t0 space3

	//创建根参数，保存描述符表
	CD3DX12_ROOT_PARAMETER slotRootParameter[8];
	slotRootParameter[0].InitAsConstantBufferView(0, 0); //pass b0
	slotRootParameter[1].InitAsConstantBufferView(1, 0); //objconstant b1
	slotRootParameter[2].InitAsConstantBufferView(2, 0); //b2
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable[0]); //tex
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable[1]); //tex
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable[2]); //tex
	slotRootParameter[6].InitAsConstantBufferView(3, 0); 
	slotRootParameter[7].InitAsDescriptorTable(1, &texTable[3]); //gbuffer

	//获取采样器
	auto staticSamplers = GetStaticSamplers();

	//根签名描述结构
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(8, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	//创建根签名
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void MxEngine::MxRenderer::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["IrradianceCubemapVS"] = d3dUtil::CompileShader(L"Shaders\\IrradianceCubemap.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["IrradianceCubemapPS"] = d3dUtil::CompileShader(L"Shaders\\IrradianceCubemap.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["PrefilterMapVS"] = d3dUtil::CompileShader(L"Shaders\\PrefilterMap.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["PrefilterMapPS"] = d3dUtil::CompileShader(L"Shaders\\PrefilterMap.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["DeferredVS"] = d3dUtil::CompileShader(L"Shaders\\Deferred.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["DeferredPS"] = d3dUtil::CompileShader(L"Shaders\\Deferred.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void MxEngine::MxRenderer::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.0f,100,100);
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(50.0f, 50.0f, 60, 40);
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT sphereVertexOffset = 0;
	UINT boxVertexOffset = (UINT)sphere.Vertices.size();
	UINT gridVertexOffset = boxVertexOffset + (UINT)box.Vertices.size();
	UINT quadVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT sphereIndexOffset = 0;
	UINT boxIndexOffset = (UINT)sphere.Indices32.size();
	UINT gridIndexOffset = boxIndexOffset + (UINT)box.Indices32.size();
	UINT quadIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();

	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.
	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry quadSubmesh;
	quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
	quadSubmesh.StartIndexLocation = quadIndexOffset;
	quadSubmesh.BaseVertexLocation = quadVertexOffset;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//
	auto totalVertexCount =
		sphere.Vertices.size() + 
		box.Vertices.size() + 
		grid.Vertices.size() +
		quad.Vertices.size();;

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].Tangent = sphere.Vertices[i].TangentU;
		vertices[k].TexCoords = sphere.Vertices[i].TexC;
	}
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].Tangent = box.Vertices[i].TangentU;
		vertices[k].TexCoords = box.Vertices[i].TexC;
	}
	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].Tangent = grid.Vertices[i].TangentU;
		vertices[k].TexCoords = grid.Vertices[i].TexC;
	}

	for (int i = 0; i < quad.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = quad.Vertices[i].Position;
		vertices[k].Normal = quad.Vertices[i].Normal;
		vertices[k].TexCoords = quad.Vertices[i].TexC;
		vertices[k].Tangent = quad.Vertices[i].TangentU;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["quad"] = quadSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MxEngine::MxRenderer::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for deferred Geometry pass.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC deferredGeoPsoDesc = opaquePsoDesc;
	deferredGeoPsoDesc.pRootSignature = mRootSignature.Get();
	deferredGeoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["DeferredVS"]->GetBufferPointer()),
		mShaders["DeferredVS"]->GetBufferSize()
	};
	deferredGeoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["DeferredPS"]->GetBufferPointer()),
		mShaders["DeferredPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&deferredGeoPsoDesc, IID_PPV_ARGS(&mPSOs["DeferredGeo"])));


	//
	// PSO for shadow map pass.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.pRootSignature = mRootSignature.Get();
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
		mShaders["shadowVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowOpaquePS"]->GetBufferPointer()),
		mShaders["shadowOpaquePS"]->GetBufferSize()
	};

	// Shadow map pass does not have a render target.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));

	//
	// PSO for debug layer.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = opaquePsoDesc;
	debugPsoDesc.pRootSignature = mRootSignature.Get();
	debugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
		mShaders["debugVS"]->GetBufferSize()
	};
	debugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
		mShaders["debugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["debug"])));

	//
	// PSO for sky.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

	// The camera is inside the sky sphere, so just turn off culling.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = mRootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
		mShaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
		mShaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC irradiancePsoDesc = skyPsoDesc;
	irradiancePsoDesc.pRootSignature = mRootSignature.Get();
	irradiancePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["IrradianceCubemapVS"]->GetBufferPointer()),
		mShaders["IrradianceCubemapVS"]->GetBufferSize()
	};
	irradiancePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["IrradianceCubemapPS"]->GetBufferPointer()),
		mShaders["IrradianceCubemapPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&irradiancePsoDesc, IID_PPV_ARGS(&mPSOs["Irradiance"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC prefilterPsoDesc = skyPsoDesc;
	prefilterPsoDesc.pRootSignature = mRootSignature.Get();
	prefilterPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["PrefilterMapVS"]->GetBufferPointer()),
		mShaders["PrefilterMapVS"]->GetBufferSize()
	};
	prefilterPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["PrefilterMapPS"]->GetBufferPointer()),
		mShaders["PrefilterMapPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&prefilterPsoDesc, IID_PPV_ARGS(&mPSOs["Prefilter"])));
}

void MxEngine::MxRenderer::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			8, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void MxEngine::MxRenderer::BuildRenderItems()
{
	auto sphereRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&sphereRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 10.0f, 0.0f));
	XMStoreFloat4x4(&sphereRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	//sphereRitem->TexTransform = MathHelper::Identity4x4();
	sphereRitem->ObjCBIndex = 0;
	sphereRitem->Mat = mMaterials["marble"].get();
	sphereRitem->Geo = mGeometries["shapeGeo"].get();
	sphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	sphereRitem->IndexCount = sphereRitem->Geo->DrawArgs["sphere"].IndexCount;
	sphereRitem->StartIndexLocation = sphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	sphereRitem->BaseVertexLocation = sphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(sphereRitem.get());
	//mRitemLayer[(int)RenderLayer::DeferredGeo].push_back(sphereRitem.get());
	mAllRitems.push_back(std::move(sphereRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&gridRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	//XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(2.0f, 1.0f, 1.0f));
	gridRitem->TexTransform = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["floor"].get();
	gridRitem->Geo = mGeometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	mAllRitems.push_back(std::move(gridRitem));

	auto skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(500, 500, 500));
	skyRitem->TexTransform = MathHelper::Identity4x4();
	skyRitem->ObjCBIndex = 2;
	skyRitem->Mat = mMaterials["sky"].get();
	skyRitem->Geo = mGeometries["shapeGeo"].get();
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["box"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["box"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
	mAllRitems.push_back(std::move(skyRitem));

	auto skyBoxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyBoxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 5.0f, 0.0f));
	skyBoxRitem->TexTransform = MathHelper::Identity4x4();
	skyBoxRitem->ObjCBIndex = 3;
	skyBoxRitem->Mat = mMaterials["sky"].get();
	skyBoxRitem->Geo = mGeometries["shapeGeo"].get();
	skyBoxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyBoxRitem->IndexCount = skyBoxRitem->Geo->DrawArgs["box"].IndexCount;
	skyBoxRitem->StartIndexLocation = skyBoxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	skyBoxRitem->BaseVertexLocation = skyBoxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Debug].push_back(skyBoxRitem.get());
	mAllRitems.push_back(std::move(skyBoxRitem));

	auto irradianceRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&irradianceRitem->World, XMMatrixScaling(10, 10, 10));
	irradianceRitem->TexTransform = MathHelper::Identity4x4();
	irradianceRitem->ObjCBIndex = 4;
	irradianceRitem->Mat = mMaterials["sky"].get();
	irradianceRitem->Geo = mGeometries["shapeGeo"].get();
	irradianceRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	irradianceRitem->IndexCount = irradianceRitem->Geo->DrawArgs["box"].IndexCount;
	irradianceRitem->StartIndexLocation = irradianceRitem->Geo->DrawArgs["box"].StartIndexLocation;
	irradianceRitem->BaseVertexLocation = irradianceRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Irradiance].push_back(irradianceRitem.get());
	mAllRitems.push_back(std::move(irradianceRitem));

	auto modelRitem = std::make_unique<RenderItem>();
	
	XMStoreFloat4x4(&modelRitem->World, XMMatrixRotationRollPitchYaw(MathHelper::Pi / 2, MathHelper::Pi ,0.0f) * XMMatrixTranslation(0.0f, 20.0f, 0.0f));
	XMStoreFloat4x4(&modelRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	//sphereRitem->TexTransform = MathHelper::Identity4x4();
	modelRitem->ObjCBIndex = 5;
	modelRitem->Mat = mMaterials["modelMat"].get();
	modelRitem->Geo = mGeometries["modelGeo"].get();
	modelRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	modelRitem->IndexCount = modelRitem->Geo->DrawArgs["model"].IndexCount;
	modelRitem->StartIndexLocation = modelRitem->Geo->DrawArgs["model"].StartIndexLocation;
	modelRitem->BaseVertexLocation = modelRitem->Geo->DrawArgs["model"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(modelRitem.get());
	mAllRitems.push_back(std::move(modelRitem));

	int offset = 6;
	for (int i = 0; i < 10; ++i)
	{
		auto sphereRitem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&sphereRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-15.0f + i * 3.0f, 10.0f, -10.0f));
		XMStoreFloat4x4(&sphereRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		//sphereRitem->TexTransform = MathHelper::Identity4x4();
		sphereRitem->ObjCBIndex = offset++;
		sphereRitem->Mat = mMaterials["defaultR" + std::to_string(i)].get();
		sphereRitem->Geo = mGeometries["shapeGeo"].get();
		sphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		sphereRitem->IndexCount = sphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		sphereRitem->StartIndexLocation = sphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		sphereRitem->BaseVertexLocation = sphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::Opaque].push_back(sphereRitem.get());
		mAllRitems.push_back(std::move(sphereRitem));
	}

	for (int i = 0; i < 10; ++i)
	{
		auto sphereRitem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&sphereRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-15.0f + i * 3.0f, 15.0f, -10.0f));
		XMStoreFloat4x4(&sphereRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		//sphereRitem->TexTransform = MathHelper::Identity4x4();
		sphereRitem->ObjCBIndex = offset++;
		sphereRitem->Mat = mMaterials["defaultM" + std::to_string(i)].get();
		sphereRitem->Geo = mGeometries["shapeGeo"].get();
		sphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		sphereRitem->IndexCount = sphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		sphereRitem->StartIndexLocation = sphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		sphereRitem->BaseVertexLocation = sphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::Opaque].push_back(sphereRitem.get());
		mAllRitems.push_back(std::move(sphereRitem));
	}


	//level items
	for (auto& object : MxWorld::GetWorld()->GetMainLevel()->LevelObjectsMap)
	{
		
		auto levelRitem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&levelRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 30.0f, 0.0f));
		XMStoreFloat4x4(&levelRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		levelRitem->ObjCBIndex = offset++;
		levelRitem->Mat = mMaterials[object.second->RenderComponent->mMaterial->Name].get();
		levelRitem->Geo = object.second->RenderComponent->mGeometrie.get();
		levelRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		levelRitem->IndexCount = sphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		levelRitem->StartIndexLocation = levelRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		levelRitem->BaseVertexLocation = levelRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::Opaque].push_back(levelRitem.get());
		mAllRitems.push_back(std::move(levelRitem));
	}


	mRitemLayer[(int)RenderLayer::DeferredGeo] = mRitemLayer[(int)RenderLayer::Opaque];
}

void MxEngine::MxRenderer::BuildCubeFaceCamera(float x, float y, float z)
{
	XMFLOAT3 center(x, y, z);

	// Look along each coordinate axis.
	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y + 1.0f, z), // +Y
		XMFLOAT3(x, y - 1.0f, z), // -Y
		XMFLOAT3(x, y, z + 1.0f), // +Z
		XMFLOAT3(x, y, z - 1.0f)  // -Z
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};

	captureProjection = DirectX::XMMatrixPerspectiveFovLH(90.0f, 1.0f, 0.1f, 10.0f);
	//captureViews[0] = MathHelper::Lookat(XMFLOAT3(0, 0, 0), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f));
	//captureViews[1] = MathHelper::Lookat(XMFLOAT3(0, 0, 0), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f));
	//captureViews[2] = MathHelper::Lookat(XMFLOAT3(0, 0, 0), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f));
	//captureViews[3] = MathHelper::Lookat(XMFLOAT3(0, 0, 0), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f));
	//captureViews[4] = MathHelper::Lookat(XMFLOAT3(0, 0, 0), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f));
	//captureViews[5] = MathHelper::Lookat(XMFLOAT3(0, 0, 0), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f));

	//for (int i = 0; i < 6; ++i)
	//{
	//	captureViews[i] = MathHelper::Lookat(XMFLOAT3(0, 0, 0), targets[i], ups[i]);
	//}

	for (int i = 0; i < 6; ++i)
	{
		mCubeMapCamera[i].LookAt(center, targets[i], ups[i]);
		mCubeMapCamera[i].SetLens(0.5f * XM_PI, 1.0f, 0.1f, 1000.0f);
		mCubeMapCamera[i].UpdateViewMatrix();
	}
}

void MxEngine::MxRenderer::BuildCubeDepthStencil()
{
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = CubeMapSize;
	depthStencilDesc.Height = CubeMapSize;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mCubeDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	md3dDevice->CreateDepthStencilView(mCubeDepthStencilBuffer.Get(), nullptr, mCubeDSV);

	// Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));


	//for (int i = 0; i < gPrefilterLevel; ++i)
	//{
	//	depthStencilDesc.Width = mPrefilterCubeMap[i]->GetWidth();
	//	depthStencilDesc.Height = mPrefilterCubeMap[i]->GetHeight();

	//	D3D12_CLEAR_VALUE optClear;
	//	optClear.Format = mDepthStencilFormat;
	//	optClear.DepthStencil.Depth = 1.0f;
	//	optClear.DepthStencil.Stencil = 0;
	//	ThrowIfFailed(md3dDevice->CreateCommittedResource(
	//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	//		D3D12_HEAP_FLAG_NONE,
	//		&depthStencilDesc,
	//		D3D12_RESOURCE_STATE_COMMON,
	//		&optClear,
	//		IID_PPV_ARGS(mPrefilterDepthStencilBuffers[i].GetAddressOf())));

	//	md3dDevice->CreateDepthStencilView(mPrefilterDepthStencilBuffers[i].Get(), nullptr, mPrefilterCubeDSVs[i]);

	//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPrefilterDepthStencilBuffers[i].Get(),
	//		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//}

}

void MxEngine::MxRenderer::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matBuffer = mCurrFrameResource->MaterialCB->Resource();
	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matBuffer->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(2, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}


}

void MxEngine::MxRenderer::DrawSceneToShadowMap()
{
	mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(mShadowMap->Dsv(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());

	// Bind the pass constant buffer for the shadow map pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void MxEngine::MxRenderer::DrawSceneToCubeMap()
{
	mCommandList->RSSetViewports(1, &mEnvCubeMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mEnvCubeMap->ScissorRect());

	// Change to RENDER_TARGET.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mEnvCubeMap->Resource(),
	//	D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	mEnvCubeMap->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// For each cube map face.
	for (int i = 0; i < 6; ++i)
	{
		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(mEnvCubeMap->Rtv(i), Colors::LightSteelBlue, 0, nullptr);
		mCommandList->ClearDepthStencilView(mCubeDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &mEnvCubeMap->Rtv(i), true, &mCubeDSV);

		// Bind the pass constant buffer for this cube map face so we use 
		// the right view/proj matrix for this cube face.
		auto passCB = mCurrFrameResource->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (2 + i) * passCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

		//DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

		mCommandList->SetPipelineState(mPSOs["sky"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Sky]);

		//mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	}

	// Change back to GENERIC_READ so we can read the texture in a shader.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mEnvCubeMap->Resource(),
	//	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mEnvCubeMap->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

}

void MxEngine::MxRenderer::DrawIrradianceCubeMap()
{
	mCommandList->RSSetViewports(1, &mIrradianceCubeMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mIrradianceCubeMap->ScissorRect());

	// Change to RENDER_TARGET.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mIrradianceCubeMap->Resource(),
	//	D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	mIrradianceCubeMap->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// For each cube map face.
	for (int i = 0; i < 6; ++i)
	{
		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(mIrradianceCubeMap->Rtv(i), Colors::LightSteelBlue, 0, nullptr);
		mCommandList->ClearDepthStencilView(mCubeDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &mIrradianceCubeMap->Rtv(i), true, &mCubeDSV);

		// Bind the pass constant buffer for this cube map face so we use 
		// the right view/proj matrix for this cube face.
		auto passCB = mCurrFrameResource->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (2 + i) * passCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

		mCommandList->SetPipelineState(mPSOs["Irradiance"].Get());

		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Sky]);
	}

	// Change back to GENERIC_READ so we can read the texture in a shader.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mIrradianceCubeMap->Resource(),
	//	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mIrradianceCubeMap->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

}

void MxEngine::MxRenderer::DrawPrefilterCubeMap()
{
	mCommandList->SetPipelineState(mPSOs["Prefilter"].Get());
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	UINT prefilterCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PrefilterConstants));
	for (int i = 0; i < gPrefilterLevel; ++i)
	{
		// For each cube map face.
		for (int j = 0; j < 6; ++j)
		{
			mCommandList->RSSetViewports(1, &mPrefilterCubeMap[i]->Viewport());
			mCommandList->RSSetScissorRects(1, &mPrefilterCubeMap[i]->ScissorRect());

			// Change to RENDER_TARGET.
			//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPrefilterCubeMap[i]->Resource(),
			//	D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
			mPrefilterCubeMap[i]->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

			// Clear the back buffer and depth buffer.
			mCommandList->ClearRenderTargetView(mPrefilterCubeMap[i]->Rtv(j), Colors::LightSteelBlue, 0, nullptr);
			//mCommandList->ClearDepthStencilView(mPrefilterCubeDSVs[i], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &mPrefilterCubeMap[i]->Rtv(j), true,nullptr);

			// Bind the pass constant buffer for this cube map face so we use 
			// the right view/proj matrix for this cube face.
			auto passCB = mCurrFrameResource->PassCB->Resource();
			D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (2 + j) * passCBByteSize;
			mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);
			
			D3D12_GPU_VIRTUAL_ADDRESS prefilterCBAddress = PrefilterCB->Resource()->GetGPUVirtualAddress() + i * prefilterCBByteSize;

			mCommandList->SetGraphicsRootConstantBufferView(6, prefilterCBAddress);

			DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Sky]);

			// Change back to GENERIC_READ so we can read the texture in a shader.
			//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPrefilterCubeMap[i]->Resource(),
			//	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
			mPrefilterCubeMap[i]->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

		}
	}

}

void MxEngine::MxRenderer::DrawGBufferMap()
{
	mCommandList->RSSetViewports(1, &mGBufferMRT->Viewport());
	mCommandList->RSSetScissorRects(1, &mGBufferMRT->ScissorRect());

	// Change state.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTarget0->Resource(),
	//	D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mGBufferMRT->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	//mCommandList->ClearRenderTargetView(mRenderTarget0->Rtv(0), Colors::LightSteelBlue, 0, nullptr);

	mGBufferMRT->RTClearView(mCommandList.Get(), Colors::LightSteelBlue, 0, nullptr);

	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(4, &mGBufferMRT->Rtv(), true, &DepthStencilView());

	// Bind the pass constant buffer for the gbuffer pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	//绘制延迟pass几何信息
	mCommandList->SetPipelineState(mPSOs["DeferredGeo"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::DeferredGeo]);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTarget0->Resource(),
	//	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mGBufferMRT->RTTransition(mCommandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> MxEngine::MxRenderer::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);


	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}

void MxEngine::MxRenderer::LoadTexture()
{
	//std::unordered_map<std::string, std::wstring> MaterialTex
	//{
	//	{"MarbleAlbedo", L"Assets/Texture/TexturesCom_Marble_TilesDiamond2_512_albedo.jpg"},
	//	{"MarbleRoughness", L"Assets/Texture/TexturesCom_Marble_TilesDiamond2_512_roughness.jpg"},
	//	{"MarbleNormal", L"Assets/Texture/TexturesCom_Marble_TilesDiamond2_512_normal.jpg"},
	//	{"rustedironAlbedo", L"Assets/Texture/rustediron2_basecolor.png"},
	//	{"rustedironRoughness", L"Assets/Texture/rustediron2_roughness.png"},
	//	{"rustedironNormal", L"Assets/Texture/rustediron2_normal.png"},
	//	{"rustedironMetallic", L"Assets/Texture/rustediron2_metallic.png"},
	//};

	MaterialTex.insert({ "MarbleAlbedo", L"Assets/Texture/TexturesCom_Marble_TilesDiamond2_512_albedo.jpg" });
	MaterialTex.insert({ "MarbleRoughness", L"Assets/Texture/TexturesCom_Marble_TilesDiamond2_512_roughness.jpg" });
	MaterialTex.insert({ "MarbleNormal", L"Assets/Texture/TexturesCom_Marble_TilesDiamond2_512_normal.jpg" });
	MaterialTex.insert({ "rustedironAlbedo", L"Assets/Texture/rustediron2_basecolor.png"});
	MaterialTex.insert({ "rustedironRoughness", L"Assets/Texture/rustediron2_roughness.png" });
	MaterialTex.insert({ "rustedironNormal", L"Assets/Texture/rustediron2_normal.png" });
	MaterialTex.insert({ "rustedironMetallic", L"Assets/Texture/rustediron2_metallic.png" });

	std::unordered_map<std::string, std::wstring> GlobalTex
	{
		{"hdrTex", L"Assets/Texture/museum_of_ethnography_4k.dds"},
		{"lut",L"Assets/Texture/ibl_brdf_lut.png"},
	};

	int index = mTextures.size();
	for (const auto& tex : MaterialTex)
	{
		auto texObj = std::make_unique<Texture>();
		texObj->Name = tex.first;
		texObj->Filename = tex.second;	
		Resource::CreateShaderResourceViewFromFile(texObj->Filename.c_str(), texObj.get());
		if (texObj->Resource)
		{
			texObj->TexHeapIndex = index++;
			mTextures[texObj->Name] = std::move(texObj);
		}
	}

	//index = 0;
	for (const auto& tex : GlobalTex)
	{
		auto texObj = std::make_unique<Texture>();
		texObj->Name = tex.first;
		texObj->Filename = tex.second;
		Resource::CreateShaderResourceViewFromFile(texObj->Filename.c_str(), texObj.get());
		if (texObj)
		{	
			texObj->TexHeapIndex = index++;
			mTextures[texObj->Name] = std::move(texObj);
		}
	}

	mTexSrvOffset = 0;
	mSkyTexSrvOffset = MaterialTex.size();

}

void MxEngine::MxRenderer::BuildMaterial()
{
	int matIndex = 0;

	auto skyMat = std::make_shared<Material>();
	skyMat->Name = "sky";
	skyMat->MatCBIndex = matIndex++;
	skyMat->AlbedoSrvHeapIndex = -1;
	skyMat->MetallicSrvHeapIndex = -1;
	skyMat->NormalSrvHeapIndex = -1;
	skyMat->RoughnessSrvHeapIndex = -1;
	skyMat->BaseColor = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	skyMat->DiffuseAlbedo = XMFLOAT4(0.5f, 0.0, 0.0, 1.0f);
	skyMat->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	skyMat->AmbientColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	skyMat->SpecularStrength = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skyMat->Roughness = 1.0f;
	skyMat->Ao = 1.0f;
	mMaterials[skyMat->Name] = std::move(skyMat);

	for (int i = 0; i < 10; ++i)
	{
		auto default = std::make_shared<Material>();
		default->Name = "defaultR" + std::to_string(i);
		default->MatCBIndex = matIndex++;
		default->AlbedoSrvHeapIndex = -1;
		default->MetallicSrvHeapIndex = -1;
		default->NormalSrvHeapIndex = -1;
		default->RoughnessSrvHeapIndex = -1;
		default->BaseColor = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
		default->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
		default->Roughness = 0.1f * i;
		default->Metallic = 0.2f;
		default->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
		default->AmbientColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		default->SpecularStrength = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		default->Ao = 1.0f;
		mMaterials[default->Name] = std::move(default);
	}


	for (int i = 0; i < 10; ++i)
	{
		auto default = std::make_shared<Material>();
		default->Name = "defaultM" + std::to_string(i);
		default->MatCBIndex = matIndex++;
		default->AlbedoSrvHeapIndex = -1;
		default->MetallicSrvHeapIndex = -1;
		default->NormalSrvHeapIndex = -1;
		default->RoughnessSrvHeapIndex = -1;
		default->BaseColor = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
		default->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
		default->Roughness = 0.3f;
		default->Metallic = 0.1f * i;
		default->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
		default->AmbientColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		default->SpecularStrength = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		default->Ao = 1.0f;
		mMaterials[default->Name] = std::move(default);
	}

	auto floor = std::make_shared<Material>();
	floor->Name = "floor";
	floor->MatCBIndex = matIndex++;
	floor->AlbedoSrvHeapIndex = mTextures["rustedironAlbedo"]->TexHeapIndex;
	floor->MetallicSrvHeapIndex = -1;
	floor->NormalSrvHeapIndex = mTextures["rustedironNormal"]->TexHeapIndex;
	floor->RoughnessSrvHeapIndex = mTextures["rustedironRoughness"]->TexHeapIndex;
	floor->BaseColor = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	floor->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	floor->Roughness = 0.25f;
	floor->Metallic = 0.2f;
	floor->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	floor->AmbientColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	floor->SpecularStrength = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	floor->Ao = 1.0f;
	mMaterials[floor->Name] = std::move(floor);

	auto marble = std::make_shared<Material>();
	marble->Name = "marble";
	marble->MatCBIndex = matIndex++;
	marble->AlbedoSrvHeapIndex = mTextures["MarbleAlbedo"]->TexHeapIndex;
	marble->MetallicSrvHeapIndex = -1;
	marble->NormalSrvHeapIndex = mTextures["MarbleNormal"]->TexHeapIndex;
	marble->RoughnessSrvHeapIndex = mTextures["MarbleRoughness"]->TexHeapIndex;
	marble->BaseColor = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	marble->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	marble->Roughness = 0.25f;
	marble->Metallic = 0.2f;
	marble->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	marble->AmbientColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	marble->SpecularStrength = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	marble->Ao = 1.0f;
	mMaterials[marble->Name] = std::move(marble);

	auto modelMat = std::make_shared<Material>();
	modelMat->Name = "modelMat";
	modelMat->MatCBIndex = matIndex++;
	modelMat->AlbedoSrvHeapIndex = mTextures["Default_albedo"]->TexHeapIndex;
	modelMat->MetallicSrvHeapIndex = -1;
	modelMat->NormalSrvHeapIndex = mTextures["Default_normal"]->TexHeapIndex;
	modelMat->RoughnessSrvHeapIndex = mTextures["metallicRoughnessTexture"]->TexHeapIndex;
	modelMat->EmissiveSrvHeapIndex = mTextures["Default_emissive"]->TexHeapIndex;
	modelMat->BaseColor = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	modelMat->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	modelMat->Roughness = 0.25f;
	modelMat->Metallic = 0.2f;
	modelMat->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	modelMat->AmbientColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	modelMat->SpecularStrength = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	modelMat->Ao = 1.0f;
	mMaterials[modelMat->Name] = std::move(modelMat);

	for (auto& object : MxWorld::GetWorld()->GetMainLevel()->LevelObjectsMap)
	{
		auto material = object.second->RenderComponent->mMaterial;
		material->MatCBIndex = matIndex++;
		mMaterials[material->Name] = std::move(material);
	}
}

void MxEngine::MxRenderer::LoadModel()
{
	Resource::LoadModelFromFile("Assets/GltfModel/DamagedHelmet.gltf");

}

HINSTANCE MxEngine::MxRenderer::AppInst() const
{
	return mhAppInst;
}

HWND MxEngine::MxRenderer::MainWnd() const
{
	return mhMainWnd;
}

float MxEngine::MxRenderer::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

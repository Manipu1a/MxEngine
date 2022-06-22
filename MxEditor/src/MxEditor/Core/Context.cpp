#include "MxEditor/Core/Context.h"

#include "MxUI/Core/UIManager.h"


MxEditor::Core::Context::Context(const std::string& p_projectPath, const std::string& p_projectName) : 
    projectPath(p_projectPath),
    projectName(p_projectName)
{
    

}

MxEditor::Core::Context::~Context()
{
    
}

void MxEditor::Core::Context::InitContext(HWND hWnd)
{
    InitD3D();
    InitCommandObject();
    mhMainWnd = hWnd;
    
    uiManager = std::make_unique<MxUI::Core::UIManager>(md3dDevice.Get(),&mhMainWnd);
}

void MxEditor::Core::Context::InitD3D()
{
    // Try to create hardware device.
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr,             // default adapter
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&md3dDevice));

    // Fallback to WARP device.
    if(FAILED(hardwareResult))
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
        mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));

        D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&md3dDevice));
    }
}

void MxEditor::Core::Context::InitCommandObject()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    (md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    (md3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

    (md3dDevice->CreateCommandList(
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

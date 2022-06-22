#include "MxUI/Core/UIManager.h"


MxUI::Core::UIManager::UIManager(ID3D12Device* device, HWND* winHandle)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mGuiSrvDescriptorHeap)) != S_OK)
        return;

    //初始化GUI
    // 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    const int gNumFrameResources = 3;
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(winHandle);
    ImGui_ImplDX12_Init(device, gNumFrameResources,
        DXGI_FORMAT_R8G8B8A8_UNORM, mGuiSrvDescriptorHeap.Get(),
        mGuiSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        mGuiSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

MxUI::Core::UIManager::~UIManager()
{
    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void MxUI::Core::UIManager::SetCanvas(Canvas& p_canvas)
{
    m_currentCanvas = &p_canvas;
}

void MxUI::Core::UIManager::Render(ID3D12GraphicsCommandList* cmdList)
{
    if (m_currentCanvas)
    {
        m_currentCanvas->Draw();
        // Rendering
        ImGui::Render();

        ID3D12DescriptorHeap* GuidescriptorHeaps[] = { mGuiSrvDescriptorHeap.Get() };
        
        cmdList->SetDescriptorHeaps(1, GuidescriptorHeaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
    }
}

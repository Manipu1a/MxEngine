#include "MxGui.h"
#include "d3dApp.h"

void MxGui::Initialize(ID3D12Device* device, HWND& winHandle)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mGuiSrvDescriptorHeap)) != S_OK)
		return;

	//³õÊ¼»¯GUI
	// 
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(winHandle);
	ImGui_ImplDX12_Init(device, gNumFrameResources,
		DXGI_FORMAT_R8G8B8A8_UNORM, mGuiSrvDescriptorHeap.Get(),
		mGuiSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		mGuiSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void MxGui::tick_pre()
{
	{
		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		onTick();
	}
}

void MxGui::onTick()
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	{
		static float fx = 0.0f;
		static float fy = 0.0f;
		static float fz = 0.0f;
		ImVec2 work_pos = viewport->WorkPos;
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = work_pos.x + 10.0f;
		window_pos.y = work_pos.y + 10.0f;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
		ImGui::Begin("GUI");
		ImGui::Text("Control Dir Light.");
		//ImGui::SliderFloat("floatx", &DirectLightX, -1.0f, 1.0f);
		//ImGui::SliderFloat("floaty", &DirectLightY, -1.0f, 1.0f);
		//ImGui::SliderFloat("floatz", &DirectLightZ, -1.0f, 10.0f);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}
}

void MxGui::tick_post()
{

}

void MxGui::draw_frame()
{
	// Rendering
	ImGui::Render();

	ID3D12DescriptorHeap* GuidescriptorHeaps[] = { mGuiSrvDescriptorHeap.Get() };
	auto commandList = D3DApp::GetApp()->GetCommandList();
	commandList->SetDescriptorHeaps(1, GuidescriptorHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void MxGui::clear()
{
	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


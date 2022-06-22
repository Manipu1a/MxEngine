#include "MxUI/Core/Canvas.h"

void MxUI::Core::Canvas::AddPanel(Panels::PanelBase& p_panel)
{
    m_panels.push_back(std::ref(p_panel));
}

void MxUI::Core::Canvas::RemovePanel(Panels::PanelBase& p_panel)
{
    m_panels.erase(std::remove_if(m_panels.begin(), m_panels.end(), [&p_panel](std::reference_wrapper<Panels::PanelBase>& p_item)
{
    return &p_panel == &p_item.get();
}));
}

void MxUI::Core::Canvas::RemoveAllPanels()
{
    m_panels.clear();
}

void MxUI::Core::Canvas::MakeDockspace(bool p_state)
{
    m_isDockspace = p_state;
}

bool MxUI::Core::Canvas::IsDockspace()
{
    return m_isDockspace;
}

void MxUI::Core::Canvas::Draw()
{
    if(!m_panels.empty())
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if(m_isDockspace)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImGui::Begin("##dockspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking);
            ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
            ImGui::SetWindowPos({ 0.f, 0.f });
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            ImGui::SetWindowSize({ (float)displaySize.x, (float)displaySize.y });
            ImGui::End();

            ImGui::PopStyleVar(3);
        }

        for(auto& panel : m_panels)
        {
            panel.get().Draw();
        }

        ImGui::Render();
    }
    
}

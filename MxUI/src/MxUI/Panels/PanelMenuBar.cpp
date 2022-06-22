#include "MxUI/Panels/PanelMenuBar.h"

void MxUI::Panels::PanelMenuBar::Draw_Impl()
{
    if (!m_widgets.empty() && ImGui::BeginMainMenuBar())
    {
        DrawWidgets();
        ImGui::EndMainMenuBar();
    }
}

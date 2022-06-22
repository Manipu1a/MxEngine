#include "MxUI/Panels/PanelBase.h"
//#include "MxUI/Imgui/imgui.h"

uint64_t MxUI::Panels::PanelBase::PANEL_ID_INCREMENT = 0;

MxUI::Panels::PanelBase::PanelBase()
{
    m_panelID = "##" + std::to_string(PANEL_ID_INCREMENT++);
}

void MxUI::Panels::PanelBase::Draw()
{
    //createw
    if(enabled)
        Draw_Impl();
}

const std::string& MxUI::Panels::PanelBase::GetPanelID() const
{
    return m_panelID;
}

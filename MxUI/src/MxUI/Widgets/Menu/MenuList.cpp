#include "MxUI/Widgets/Menu/MenuList.h"

MxUI::Widgets::Menu::MenuList::MenuList(const std::string& p_name, bool p_locked) :
name(p_name), locked(p_locked)
{
    
}

void MxUI::Widgets::Menu::MenuList::Draw_Impl()
{
    if (ImGui::BeginMenu(name.c_str(), !locked))
    {
        if (!m_opened)
        {
           // ClickedEvent.Invoke();
            m_opened = true;
        }
        
        Group::Draw_Impl();
        ImGui::EndMenu();
    }
    else
    {
        m_opened = false;
    }
}

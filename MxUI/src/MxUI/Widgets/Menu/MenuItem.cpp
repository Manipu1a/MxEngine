#include "MxUI/Widgets/Menu/MenuItem.h"

MxUI::Widgets::Menu::MenuItem::MenuItem(const std::string& p_name, const std::string& p_shortcut, bool p_checkable,
    bool p_checked) :
DataWidget(m_selected), name(p_name), shortcut(p_shortcut), checkable(p_checkable), checked(p_checked)
{
    
}

void MxUI::Widgets::Menu::MenuItem::Draw_Impl()
{
}

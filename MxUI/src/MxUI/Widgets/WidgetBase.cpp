#include "MxUI/Widgets/WidgetBase.h"

uint64_t MxUI::Widgets::WidgetBase::WIDGET_ID_INCREMENT = 0;

MxUI::Widgets::WidgetBase::WidgetBase()
{
    m_widgetID = "##" + std::to_string(WIDGET_ID_INCREMENT++);
}

void MxUI::Widgets::WidgetBase::Draw()
{
    if (enabled)
    {
        Draw_Impl();
        
        if (!lineBreak)
            ImGui::SameLine();
    }
}

void MxUI::Widgets::WidgetBase::LinkTo(const WidgetBase& p_widget)
{
    m_widgetID = p_widget.m_widgetID;
}

void MxUI::Widgets::WidgetBase::Destory()
{
    m_destroyed = true;
}

bool MxUI::Widgets::WidgetBase::IsDestroyed() const
{
    return m_destroyed;
}

void MxUI::Widgets::WidgetBase::SetParent(Internal::WidgetContainer* p_parent)
{
    m_parent = p_parent;
}

bool MxUI::Widgets::WidgetBase::HasParent() const
{
    return m_parent;
}

MxUI::Internal::WidgetContainer* MxUI::Widgets::WidgetBase::GetParent()
{
    return m_parent;
}

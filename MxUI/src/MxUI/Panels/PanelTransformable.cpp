#include "MxUI/Panels/PanelTransformable.h"
#include "MxUI/Internal/Converter.h"

MxUI::Panels::PanelTransformable::PanelTransformable(const Eigen::Vector2f& p_defaultPosition,
    const Eigen::Vector2f& p_defaultSize, UICommon::EHorizontalAlignment p_defaultHorizontalAlignment,
    UICommon::EVerticalAlignment p_defaultVerticalAlignment, bool p_ignoreConfigFile) :
    m_defaultPosition(p_defaultPosition),
    m_defaultSize(p_defaultSize),
    m_defaultHorizontalAlignment(p_defaultHorizontalAlignment),
    m_defaultVerticalAlignment(p_defaultVerticalAlignment),
    m_ignoreConfigFile(p_ignoreConfigFile)
{
    
}

void MxUI::Panels::PanelTransformable::SetPosition(const Eigen::Vector2f& p_position)
{
    m_position = p_position;
    m_positionChanged = true;
}

void MxUI::Panels::PanelTransformable::SetSize(const Eigen::Vector2f& p_size)
{
    m_size = p_size;
    m_sizeChanged = true;
}

void MxUI::Panels::PanelTransformable::SetAlighment(UICommon::EHorizontalAlignment p_horizontalAlignment,
    UICommon::EVerticalAlignment p_verticalAlignment)
{
    m_horizontalAlignment = p_horizontalAlignment;
    m_verticalAlignment = p_verticalAlignment;
    m_alignmentChanged = true;
}

const Eigen::Vector2f& MxUI::Panels::PanelTransformable::GetPosition() const
{
    return m_position;
}

const Eigen::Vector2f& MxUI::Panels::PanelTransformable::GetSize() const
{
    return m_size;
}

MxUI::UICommon::EHorizontalAlignment MxUI::Panels::PanelTransformable::GetHorizontalAlignment()
{
    return m_horizontalAlignment;
}

MxUI::UICommon::EVerticalAlignment MxUI::Panels::PanelTransformable::GetVerticalAlignment()
{
    return m_verticalAlignment;
}

void MxUI::Panels::PanelTransformable::Update()
{
    if (!m_firstFrame)
    {
        if (!autoSize)
            UpdateSize();
        CopyImGuiSize();

        UpdatePosition();
        CopyImGuiPosition();
    }

    m_firstFrame = false;
}

Eigen::Vector2f MxUI::Panels::PanelTransformable::CalculatePositionAlignmentOffset(bool p_default)
{
    Eigen::Vector2f result(0.0f, 0.0f);

    switch (p_default ? m_defaultHorizontalAlignment : m_horizontalAlignment)
    {
    case MxUI::UICommon::EHorizontalAlignment::CENTER:
        result.x() -= m_size.x() / 2.0f;
        break;
    case MxUI::UICommon::EHorizontalAlignment::RIGHT:
        result.x() -= m_size.x();
        break;
    }

    switch (p_default ? m_defaultVerticalAlignment : m_verticalAlignment)
    {
    case MxUI::UICommon::EVerticalAlignment::MIDDLE:
        result.y() -= m_size.y() / 2.0f;
        break;
    case MxUI::UICommon::EVerticalAlignment::BOTTOM:
        result.y() -= m_size.y();
        break;
    }

    return result;
}

void MxUI::Panels::PanelTransformable::UpdatePosition()
{
    if (m_defaultPosition.x() != -1.f && m_defaultPosition.y() != 1.f)
    {
        Eigen::Vector2f offsettedDefaultPos = m_defaultPosition + CalculatePositionAlignmentOffset(true);
        ImGui::SetWindowPos(Internal::Converter::ToImVec2(offsettedDefaultPos), m_ignoreConfigFile ? ImGuiCond_Once : ImGuiCond_FirstUseEver);
    }

    if (m_positionChanged || m_alignmentChanged)
    {
        Eigen::Vector2f offset = CalculatePositionAlignmentOffset(false);
        Eigen::Vector2f offsettedPos(m_position + offset);
        ImGui::SetWindowPos(Internal::Converter::ToImVec2(offsettedPos), ImGuiCond_Always);
        m_positionChanged = false;
        m_alignmentChanged = false;
    }
}

void MxUI::Panels::PanelTransformable::UpdateSize()
{
    if (m_sizeChanged)
    {
        ImGui::SetWindowSize(Internal::Converter::ToImVec2(m_size), ImGuiCond_Always);
        m_sizeChanged = false;
    }
}

void MxUI::Panels::PanelTransformable::CopyImGuiPosition()
{
    m_position = Internal::Converter::ToVector2(ImGui::GetWindowPos());
}

void MxUI::Panels::PanelTransformable::CopyImGuiSize()
{
    m_size = Internal::Converter::ToVector2(ImGui::GetWindowSize());
}

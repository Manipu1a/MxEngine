#pragma once
#include "MxUI/Panels/PanelBase.h"
#include <MxMath/Eigen/Dense>
#include "MxUI/Common/UICommon.h"

namespace MxUI::Panels
{
    class PanelTransformable : public PanelBase
    {
    public:
        PanelTransformable(	const Eigen::Vector2f& p_defaultPosition = Eigen::Vector2f(-1.f, -1.f),
            const Eigen::Vector2f& p_defaultSize = Eigen::Vector2f(-1.f, -1.f),
            UICommon::EHorizontalAlignment p_defaultHorizontalAlignment = UICommon::EHorizontalAlignment::LEFT,
            UICommon::EVerticalAlignment p_defaultVerticalAlignment = UICommon::EVerticalAlignment::TOP,
            bool p_ignoreConfigFile = false);

        void SetPosition(const Eigen::Vector2f& p_position);

        void SetSize(const Eigen::Vector2f& p_size);

        void SetAlighment(UICommon::EHorizontalAlignment p_horizontalAlignment, UICommon::EVerticalAlignment p_verticalAlignment);

        const Eigen::Vector2f& GetPosition() const;

        const Eigen::Vector2f& GetSize() const;

        UICommon::EHorizontalAlignment GetHorizontalAlignment();

        UICommon::EVerticalAlignment GetVerticalAlignment();

    protected:
        void Update();
        virtual void Draw_Impl() = 0;
        
    private:
        Eigen::Vector2f CalculatePositionAlignmentOffset(bool p_default = false);

        void UpdatePosition();
        void UpdateSize();
        void CopyImGuiPosition();
        void CopyImGuiSize();

    public:
        bool autoSize = true;

    protected:
        Eigen::Vector2f m_defaultPosition;
        Eigen::Vector2f m_defaultSize;
        UICommon::EHorizontalAlignment m_defaultHorizontalAlignment;
        UICommon::EVerticalAlignment m_defaultVerticalAlignment;
        bool m_ignoreConfigFile;
        
        Eigen::Vector2f m_position = Eigen::Vector2f(0.0f, 0.0f);
        Eigen::Vector2f m_size = Eigen::Vector2f(0.0f, 0.0f);

        bool m_positionChanged = false;
        bool m_sizeChanged = false;

        UICommon::EHorizontalAlignment m_horizontalAlignment = UICommon::EHorizontalAlignment::LEFT;
        UICommon::EVerticalAlignment m_verticalAlignment = UICommon::EVerticalAlignment::TOP;

        bool m_alignmentChanged = false;
        bool m_firstFrame = true;
    };
}

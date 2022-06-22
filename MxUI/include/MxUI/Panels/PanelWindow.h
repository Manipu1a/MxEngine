#pragma once

#include "MxUI/Panels/PanelTransformable.h"

namespace MxUI::Panels
{
    class PanelWindow : public PanelTransformable
    {
    public:

        PanelWindow(const std::string& p_name = "", bool p_opened = true,
            const UICommon::PanelWindowSettings& p_panelSettings = UICommon::PanelWindowSettings{});

        /**
        * Open (show) the panel
        */
        void Open();

        /**
        * Close (hide) the panel
        */
        void Close();

        /**
        * Focus the panel
        */
        void Focus();

        /**
        * Defines the opened state of the window
        * @param p_value
        */
        void SetOpened(bool p_value);

        /**
        * Returns true if the panel is opened
        */
        bool IsOpened() const;

        /**
        * Returns true if the panel is hovered
        */
        bool IsHovered() const;

        /**
        * Returns true if the panel is focused
        */
        bool IsFocused() const;

        /**
        * Returns true if the panel is appearing
        */
        bool IsAppearing() const;

        /**
        * Scrolls to the bottom of the window
        */
        void ScrollToBottom();

        /**
        * Scrolls to the top of the window
        */
        void ScrollToTop();

        /**
        * Returns true if the window is scrolled to the bottom
        */
        bool IsScrolledToBottom() const;

        /**
        * Returns true if the window is scrolled to the bottom
        */
        bool IsScrolledToTop() const;

    private:
        void Draw_Impl() override;

    public:
        std::string name;

        Eigen::Vector2f minSize = {0.f, 0.f};
        Eigen::Vector2f maxSize = {0.f, 0.f};

        bool resizable = true;
        bool closable = false;
        bool movable = true;
        bool scrollable = true;
        bool dockable = false;
        bool hideBackground = false;
        bool forceHorizontalScrollbar = false;
        bool forceVerticalScrollbar = false;
        bool allowHorizontalScrollbar = false;
        bool bringToFrontOnFocus = true;
        bool collapsable = false;
        bool allowInputs = true;
        bool titleBar = true;

        //Event
        
    private:
        bool m_opened;
        bool m_hovered;
        bool m_focused;
        bool m_mustScrollToBottom = false;
        bool m_mustScrollToTop = false;
        bool m_scrolledToBottom = false;
        bool m_scrolledToTop = false;
    };
    
}

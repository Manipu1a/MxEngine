#pragma once

#include <vector>
#include <memory>

#include "MxUI/Interface/Drawable.h"
#include "MxUI/Panels/PanelBase.h"

#include "MxUI/Imgui/imgui.h"
#include "MxUI/Imgui/imgui_impl_dx12.h"
#include "MxUI/Imgui/imgui_impl_win32.h"

namespace MxUI::Core
{
    class Canvas : public Interface::IDrawable
    {
    public:
        void AddPanel(Panels::PanelBase& p_panel);

        void RemovePanel(Panels::PanelBase& p_panel);

        void RemoveAllPanels();

        void MakeDockspace(bool p_state);

        bool IsDockspace();

        void Draw() override;
        
    private:
        std::vector<std::reference_wrapper<Panels::PanelBase>> m_panels;
        bool m_isDockspace = false;
    };
    
}

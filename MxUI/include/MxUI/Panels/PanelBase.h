#pragma once

#include <string>

#include "MxUI/Interface/Drawable.h"
#include "MxUI/Internal/WidgetContainer.h"

namespace MxUI::Panels
{
    class PanelBase : public Interface::IDrawable, public Internal::WidgetContainer
    {
    public:
        PanelBase();
        
        void Draw();
        
        const std::string& GetPanelID() const;

    protected:
        virtual void Draw_Impl() = 0;

    public:
        bool enabled = true;

    protected:
        std::string m_panelID;

    private:
        static uint64_t PANEL_ID_INCREMENT;
    };
    
}

#pragma once

#include <string>
#include "MxUI/Interface/Drawable.h"
#include "MxUI/Plugins/DataDispatcher.h"
#include "MxUI/Imgui/imgui.h"
#include "MxUI/Internal/Color.h"
#include "MxUI/Plugins/Pluginable.h"

namespace MxUI::Internal
{
    class WidgetContainer;
}

namespace MxUI::Widgets
{
    class WidgetBase : Interface::IDrawable, public Plugins::Pluginable
    {
    public:
        WidgetBase();
        
        virtual void Draw() override;

        void LinkTo(const WidgetBase& p_widget);

        void Destory();

        bool IsDestroyed()const;

        void SetParent(Internal::WidgetContainer* p_parent);

        bool HasParent() const;

        Internal::WidgetContainer* GetParent();

    protected:
        virtual void Draw_Impl() = 0;

    public:
        bool enabled = true;
        bool lineBreak = true;

    protected:
        Internal::WidgetContainer* m_parent;
        std::string m_widgetID = "?";
        bool m_autoExecutePlugins = true;

    private:
        static uint64_t WIDGET_ID_INCREMENT;
        bool m_destroyed = false;
    };
    
}

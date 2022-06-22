#pragma once

#include "MxUI/Plugins/IPlugin.h"
#include "MxUI/Internal/WidgetContainer.h"

namespace MxUI::Plugins
{
    /**
    * A simple plugin that will show a contextual menu on right click
    * You can add widgets to a contextual menu
    */
    class ContextualMenu : public IPlugin, public Internal::WidgetContainer
    {
    public:
        /**
        * Execute the plugin
        */
        void Execute();

        /**
        * Force close the contextual menu
        */
        void Close();
    };
}

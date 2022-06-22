#pragma once

#include "MxUI/Internal/WidgetContainer.h"

namespace MxUI::Widgets::Layout
{
    /**
    * Widget that can contains other widgets
    */
    class Group : public WidgetBase, public Internal::WidgetContainer
    {
    protected:
        virtual void Draw_Impl() override;
    };
}

#pragma once

#include "MxUI/Panels/PanelBase.h"
#include "MxUI/Widgets/Menu/MenuList.h"

namespace MxUI::Panels
{
    class PanelMenuBar : public MxUI::Panels::PanelBase
    {
    protected:
        void Draw_Impl() override;
    };
    
}

#pragma once

#include "MxUI/Panels/PanelTransformable.h"

namespace MxUI::Panels
{
    class PanelUndecorated : public PanelTransformable
    {
    public:
        void Draw_Impl() override;

    private:
        int CollectFlags();
    };
}

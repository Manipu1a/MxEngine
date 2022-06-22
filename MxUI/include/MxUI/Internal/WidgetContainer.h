#pragma once

#include <functional>
#include <vector>

#include "MxUI/Common/UICommon.h"
#include "MxUI/Widgets/WidgetBase.h"

namespace MxUI::Internal
{
    class WidgetContainer
    {
    public:
        void RemoveWidget(Widgets::WidgetBase& p_widget);

        void RemoveAllWidgets();

        void ConsiderWidget(Widgets::WidgetBase& p_widget, bool p_manageMemory = true);

        void UnconsiderWidget(Widgets::WidgetBase& p_widget);

        void CollectGarbages();

        void DrawWidgets();

        void ReverseDrawOrder(bool reversed);

        template <typename T, typename ... Args>
        T& CreateWidget(Args&&... p_args)
        {
            m_widgets.emplace_back(new T(p_args...), UICommon::EMemoryMode::INTERNAL_MANAGMENT);
            T& instance = *reinterpret_cast<T*>(m_widgets.back().first);
            instance.SetParent(this);
            return instance;
        }

        std::vector<std::pair<Widgets::WidgetBase*, UICommon::EMemoryMode>>& GetWidgets();
    protected:
        std::vector<std::pair<Widgets::WidgetBase*, UICommon::EMemoryMode>> m_widgets;
        bool m_reversedDrawOrder = false;
    };
    
}

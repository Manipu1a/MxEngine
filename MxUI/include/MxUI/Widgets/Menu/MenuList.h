#pragma once

#include "MxUI/Widgets/Layout/Group.h"

namespace MxUI::Widgets::Menu
{
    /**
    * Widget that behave like a group with a menu display
    */
    class MenuList : public Layout::Group
    {
    public:
        /**
        * Constructor
        * @param p_name
        * @param p_locked
        */
        MenuList(const std::string& p_name, bool p_locked = false);

    protected:
        virtual void Draw_Impl() override;

    public:
        std::string name;
        bool locked;
        //OvTools::Eventing::Event<> ClickedEvent;

    private:
        bool m_opened;
    };
}

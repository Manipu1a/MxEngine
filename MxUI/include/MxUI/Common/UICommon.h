
#pragma once

namespace MxUI::UICommon
{
    /**
    * Defines horizontal alignments
    */
    enum class EHorizontalAlignment
    {
        LEFT,
        CENTER,
        RIGHT
    };

    /**
    * Defines vertical alignments
    */
    enum class EVerticalAlignment
    {
        TOP,
        MIDDLE,
        BOTTOM
    };

    /**
    * Data structure to send to the panel window constructor to define its settings
    */
    struct PanelWindowSettings
    {
        bool closable					= false;
        bool resizable					= true;
        bool movable					= true;
        bool dockable					= false;
        bool scrollable					= true;
        bool hideBackground				= false;
        bool forceHorizontalScrollbar	= false;
        bool forceVerticalScrollbar		= false;
        bool allowHorizontalScrollbar	= false;
        bool bringToFrontOnFocus		= true;
        bool collapsable				= false;
        bool allowInputs				= true;
        bool titleBar					= true;
        bool autoSize					= false;
    };

    /**
    * Defines how the memory should be managed
    */
    enum class EMemoryMode
    {
        INTERNAL_MANAGMENT,
        EXTERNAL_MANAGMENT
    };
}

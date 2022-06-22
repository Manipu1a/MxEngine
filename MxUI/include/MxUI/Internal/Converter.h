#pragma once

#include <MxMath/Eigen/Dense>
#include "MxUI/Imgui/imgui.h"
#include "MxUI/Internal/Color.h"

namespace MxUI::Internal
{
    class Converter
    {
    public:
        Converter() = delete;

        static ImVec4 ToImVec4(const Internal::Color& p_value);

        static Internal::Color ToColor(const ImVec4& p_value);

        static ImVec2 ToImVec2(const Eigen::Vector2f& p_value);

        static Eigen::Vector2f ToVector2(const ImVec2& p_value);
        
    };
    
}

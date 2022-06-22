#include "MxUI/Internal/Converter.h"

ImVec4 MxUI::Internal::Converter::ToImVec4(const Internal::Color& p_value)
{
    return ImVec4(p_value.r, p_value.g, p_value.b, p_value.a);
}

MxUI::Internal::Color MxUI::Internal::Converter::ToColor(const ImVec4& p_value)
{
    return Internal::Color(p_value.x, p_value.y, p_value.z, p_value.w);
}

ImVec2 MxUI::Internal::Converter::ToImVec2(const Eigen::Vector2f& p_value)
{
    return ImVec2(p_value.x(), p_value.y());
}

Eigen::Vector2f MxUI::Internal::Converter::ToVector2(const ImVec2& p_value)
{
    return Eigen::Vector2f(p_value.x, p_value.y);
}

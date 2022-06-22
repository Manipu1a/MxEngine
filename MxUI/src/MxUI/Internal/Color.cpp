#include "MxUI/Internal/Color.h"

const MxUI::Internal::Color MxUI::Internal::Color::Red		= { 1.f, 0.f, 0.f };
const MxUI::Internal::Color MxUI::Internal::Color::Green		= { 0.f, 1.f, 0.f };
const MxUI::Internal::Color MxUI::Internal::Color::Blue		= { 0.f, 0.f, 1.f };
const MxUI::Internal::Color MxUI::Internal::Color::White		= { 1.f, 1.f, 1.f };
const MxUI::Internal::Color MxUI::Internal::Color::Black		= { 0.f, 0.f, 0.f };
const MxUI::Internal::Color MxUI::Internal::Color::Grey		= { 0.5f, 0.5f, 0.5f };
const MxUI::Internal::Color MxUI::Internal::Color::Yellow		= { 1.f, 1.f, 0.f };
const MxUI::Internal::Color MxUI::Internal::Color::Cyan		= { 0.f, 1.f, 1.f };
const MxUI::Internal::Color MxUI::Internal::Color::Magenta	= { 1.f, 0.f, 1.f };

MxUI::Internal::Color::Color(float p_r, float p_g, float p_b, float p_a) : r(p_r), g(p_g), b(p_b),a(p_a)
{
}

bool MxUI::Internal::Color::operator==(const Color& p_other)
{
    return this->r == p_other.r && this->g == p_other.g && this->b == p_other.b && this->a == p_other.a;
}

bool MxUI::Internal::Color::operator!=(const Color& p_other)
{
    return !operator==(p_other);
}

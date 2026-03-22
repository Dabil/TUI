#include "Rendering/Styles/Color.h"

Color::Color()
    : m_kind(Kind::Default)
{
}

Color Color::Default()
{
    return Color(Kind::Default);
}

Color Color::FromBasic(Basic basic)
{
    return Color(basic);
}

Color Color::FromIndexed256(std::uint8_t index)
{
    return Color(index);
}

Color Color::FromRgb(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
{
    return Color(red, green, blue);
}

Color Color::FromTrueColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
{
    return Color(red, green, blue);
}

Color::Kind Color::kind() const
{
    return m_kind;
}

bool Color::isDefault() const
{
    return m_kind == Kind::Default;
}

bool Color::isBasic() const
{
    return m_kind == Kind::Basic;
}

bool Color::isIndexed256() const
{
    return m_kind == Kind::Indexed256;
}

bool Color::isRgb() const
{
    return m_kind == Kind::Rgb;
}

bool Color::isTrueColor() const
{
    return isRgb();
}

Color::Basic Color::basic() const
{
    return m_basic;
}

std::uint8_t Color::index256() const
{
    return m_index256;
}

std::uint8_t Color::red() const
{
    return m_red;
}

std::uint8_t Color::green() const
{
    return m_green;
}

std::uint8_t Color::blue() const
{
    return m_blue;
}

bool Color::isBrightBasic() const
{
    if (m_kind != Kind::Basic)
    {
        return false;
    }

    switch (m_basic)
    {
    case Basic::BrightBlack:
    case Basic::BrightRed:
    case Basic::BrightGreen:
    case Basic::BrightYellow:
    case Basic::BrightBlue:
    case Basic::BrightMagenta:
    case Basic::BrightCyan:
    case Basic::BrightWhite:
        return true;

    case Basic::Black:
    case Basic::Red:
    case Basic::Green:
    case Basic::Yellow:
    case Basic::Blue:
    case Basic::Magenta:
    case Basic::Cyan:
    case Basic::White:
    default:
        return false;
    }
}

bool Color::operator==(const Color& other) const
{
    return m_kind == other.m_kind
        && m_basic == other.m_basic
        && m_index256 == other.m_index256
        && m_red == other.m_red
        && m_green == other.m_green
        && m_blue == other.m_blue;
}

bool Color::operator!=(const Color& other) const
{
    return !(*this == other);
}

Color::Color(Kind kind)
    : m_kind(kind)
{
}

Color::Color(Basic basic)
    : m_kind(Kind::Basic)
    , m_basic(basic)
{
}

Color::Color(std::uint8_t index256)
    : m_kind(Kind::Indexed256)
    , m_index256(index256)
{
}

Color::Color(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
    : m_kind(Kind::Rgb)
    , m_red(red)
    , m_green(green)
    , m_blue(blue)
{
}
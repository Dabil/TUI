#pragma once

#include <cstdint>

class Color
{
public:
    enum class Kind
    {
        Default,
        Basic,
        Indexed256,
        TrueColor
    };

    enum class Basic
    {
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,

        BrightBlack,
        BrightRed,
        BrightGreen,
        BrightYellow,
        BrightBlue,
        BrightMagenta,
        BrightCyan,
        BrightWhite
    };

public:
    Color();

    static Color Default();
    static Color FromBasic(Basic basic);
    static Color FromIndexed256(std::uint8_t index);
    static Color FromTrueColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

    // Compatibility helper for older call sites that still use RGB naming.
    static Color FromRgb(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

    Kind kind() const;

    bool isDefault() const;
    bool isBasic() const;
    bool isIndexed256() const;
    bool isTrueColor() const;

    // Compatibility helper for older call sites that still use RGB naming.
    bool isRgb() const;

    Basic basic() const;
    std::uint8_t index256() const;
    std::uint8_t red() const;
    std::uint8_t green() const;
    std::uint8_t blue() const;

    bool isBrightBasic() const;

    bool operator==(const Color& other) const;
    bool operator!=(const Color& other) const;

private:
    explicit Color(Kind kind);
    explicit Color(Basic basic);
    explicit Color(std::uint8_t index256);
    Color(Kind kind, std::uint8_t red, std::uint8_t green, std::uint8_t blue);

private:
    Kind m_kind = Kind::Default;
    Basic m_basic = Basic::White;
    std::uint8_t m_index256 = 0;
    std::uint8_t m_red = 0;
    std::uint8_t m_green = 0;
    std::uint8_t m_blue = 0;
};

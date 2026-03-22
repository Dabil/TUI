#include "Rendering/Styles/StylePolicy.h"

namespace
{
    int squareDistance(
        std::uint8_t r1, std::uint8_t g1, std::uint8_t b1,
        std::uint8_t r2, std::uint8_t g2, std::uint8_t b2)
    {
        const int dr = static_cast<int>(r1) - static_cast<int>(r2);
        const int dg = static_cast<int>(g1) - static_cast<int>(g2);
        const int db = static_cast<int>(b1) - static_cast<int>(b2);
        return dr * dr + dg * dg + db * db;
    }

    std::uint8_t cubeValue(int component)
    {
        static const std::uint8_t values[6] = { 0, 95, 135, 175, 215, 255 };
        return values[component];
    }
}

StylePolicy::StylePolicy() = default;

StylePolicy StylePolicy::PreserveIntent()
{
    return StylePolicy{};
}

StylePolicy StylePolicy::BasicConsole()
{
    StylePolicy policy;
    policy.m_basicColorMode = ColorRenderMode::Direct;
    policy.m_indexed256ColorMode = ColorRenderMode::DowngradeToBasic;
    policy.m_rgbColorMode = ColorRenderMode::DowngradeToBasic;

    policy.m_boldMode = TextAttributeRenderMode::Direct;
    policy.m_dimMode = TextAttributeRenderMode::Direct;
    policy.m_underlineMode = TextAttributeRenderMode::Omit;
    policy.m_reverseMode = TextAttributeRenderMode::Direct;
    policy.m_invisibleMode = TextAttributeRenderMode::Direct;
    policy.m_strikeMode = TextAttributeRenderMode::Omit;

    policy.m_slowBlinkMode = BlinkRenderMode::Omit;
    policy.m_fastBlinkMode = BlinkRenderMode::Omit;

    return policy;
}

StylePolicy StylePolicy::BasicConsoleWithBlinkEmulation()
{
    StylePolicy policy = BasicConsole();
    policy.m_slowBlinkMode = BlinkRenderMode::Emulate;
    policy.m_fastBlinkMode = BlinkRenderMode::Emulate;
    return policy;
}

ResolvedStyle StylePolicy::resolve(const Style& style) const
{
    ResolvedStyle result;
    result.presentedStyle = style;

    if (result.presentedStyle.hasForeground())
    {
        const std::optional<Color> resolved = resolveColor(result.presentedStyle.foreground());
        if (resolved.has_value())
        {
            result.presentedStyle = result.presentedStyle.withForeground(*resolved);
        }
        else
        {
            result.presentedStyle = result.presentedStyle.withoutForeground();
        }
    }

    if (result.presentedStyle.hasBackground())
    {
        const std::optional<Color> resolved = resolveColor(result.presentedStyle.background());
        if (resolved.has_value())
        {
            result.presentedStyle = result.presentedStyle.withBackground(*resolved);
        }
        else
        {
            result.presentedStyle = result.presentedStyle.withoutBackground();
        }
    }

    if (result.presentedStyle.bold() && m_boldMode == TextAttributeRenderMode::Omit)
    {
        result.presentedStyle = result.presentedStyle.withBold(false);
    }

    if (result.presentedStyle.dim() && m_dimMode == TextAttributeRenderMode::Omit)
    {
        result.presentedStyle = result.presentedStyle.withDim(false);
    }

    if (result.presentedStyle.underline() && m_underlineMode == TextAttributeRenderMode::Omit)
    {
        result.presentedStyle = result.presentedStyle.withUnderline(false);
    }

    if (result.presentedStyle.reverse() && m_reverseMode == TextAttributeRenderMode::Omit)
    {
        result.presentedStyle = result.presentedStyle.withReverse(false);
    }

    if (result.presentedStyle.invisible() && m_invisibleMode == TextAttributeRenderMode::Omit)
    {
        result.presentedStyle = result.presentedStyle.withInvisible(false);
    }

    if (result.presentedStyle.strike() && m_strikeMode == TextAttributeRenderMode::Omit)
    {
        result.presentedStyle = result.presentedStyle.withStrike(false);
    }

    if (result.presentedStyle.slowBlink())
    {
        if (m_slowBlinkMode == BlinkRenderMode::Omit)
        {
            result.presentedStyle = result.presentedStyle.withSlowBlink(false);
        }
        else if (m_slowBlinkMode == BlinkRenderMode::Emulate)
        {
            result.presentedStyle = result.presentedStyle.withSlowBlink(false);
            result.emulateSlowBlink = true;
        }
    }

    if (result.presentedStyle.fastBlink())
    {
        if (m_fastBlinkMode == BlinkRenderMode::Omit)
        {
            result.presentedStyle = result.presentedStyle.withFastBlink(false);
        }
        else if (m_fastBlinkMode == BlinkRenderMode::Emulate)
        {
            result.presentedStyle = result.presentedStyle.withFastBlink(false);
            result.emulateFastBlink = true;
        }
    }

    return result;
}

ColorRenderMode StylePolicy::basicColorMode() const
{
    return m_basicColorMode;
}

ColorRenderMode StylePolicy::indexed256ColorMode() const
{
    return m_indexed256ColorMode;
}

ColorRenderMode StylePolicy::rgbColorMode() const
{
    return m_rgbColorMode;
}

TextAttributeRenderMode StylePolicy::boldMode() const
{
    return m_boldMode;
}

TextAttributeRenderMode StylePolicy::dimMode() const
{
    return m_dimMode;
}

TextAttributeRenderMode StylePolicy::underlineMode() const
{
    return m_underlineMode;
}

TextAttributeRenderMode StylePolicy::reverseMode() const
{
    return m_reverseMode;
}

TextAttributeRenderMode StylePolicy::invisibleMode() const
{
    return m_invisibleMode;
}

TextAttributeRenderMode StylePolicy::strikeMode() const
{
    return m_strikeMode;
}

BlinkRenderMode StylePolicy::slowBlinkMode() const
{
    return m_slowBlinkMode;
}

BlinkRenderMode StylePolicy::fastBlinkMode() const
{
    return m_fastBlinkMode;
}

StylePolicy StylePolicy::withBasicColorMode(ColorRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_basicColorMode = mode;
    return copy;
}

StylePolicy StylePolicy::withIndexed256ColorMode(ColorRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_indexed256ColorMode = mode;
    return copy;
}

StylePolicy StylePolicy::withRgbColorMode(ColorRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_rgbColorMode = mode;
    return copy;
}

StylePolicy StylePolicy::withBoldMode(TextAttributeRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_boldMode = mode;
    return copy;
}

StylePolicy StylePolicy::withDimMode(TextAttributeRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_dimMode = mode;
    return copy;
}

StylePolicy StylePolicy::withUnderlineMode(TextAttributeRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_underlineMode = mode;
    return copy;
}

StylePolicy StylePolicy::withReverseMode(TextAttributeRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_reverseMode = mode;
    return copy;
}

StylePolicy StylePolicy::withInvisibleMode(TextAttributeRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_invisibleMode = mode;
    return copy;
}

StylePolicy StylePolicy::withStrikeMode(TextAttributeRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_strikeMode = mode;
    return copy;
}

StylePolicy StylePolicy::withSlowBlinkMode(BlinkRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_slowBlinkMode = mode;
    return copy;
}

StylePolicy StylePolicy::withFastBlinkMode(BlinkRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_fastBlinkMode = mode;
    return copy;
}

std::optional<Color> StylePolicy::resolveColor(const std::optional<Color>& color) const
{
    if (!color.has_value())
    {
        return std::nullopt;
    }

    if (color->isDefault())
    {
        return *color;
    }

    if (color->isBasic())
    {
        return applyColorMode(*color, m_basicColorMode);
    }

    if (color->isIndexed256())
    {
        return applyColorMode(*color, m_indexed256ColorMode);
    }

    if (color->isRgb())
    {
        return applyColorMode(*color, m_rgbColorMode);
    }

    return *color;
}

std::optional<Color> StylePolicy::applyColorMode(const Color& color, ColorRenderMode mode) const
{
    switch (mode)
    {
    case ColorRenderMode::Direct:
        return color;

    case ColorRenderMode::DowngradeToBasic:
        return downgradeToBasic(color);

    case ColorRenderMode::DowngradeToIndexed256:
        return downgradeToIndexed256(color);

    case ColorRenderMode::Omit:
        return std::nullopt;

    default:
        return color;
    }
}

Color StylePolicy::downgradeToBasic(const Color& color) const
{
    if (color.isDefault() || color.isBasic())
    {
        return color;
    }

    const RgbColor rgb = toRgb(color);
    return Color::FromBasic(rgbToNearestBasic(rgb.red, rgb.green, rgb.blue));
}

Color StylePolicy::downgradeToIndexed256(const Color& color) const
{
    if (color.isDefault() || color.isIndexed256())
    {
        return color;
    }

    if (color.isBasic())
    {
        switch (color.basic())
        {
        case Color::Basic::Black:         return Color::FromIndexed256(0);
        case Color::Basic::Red:           return Color::FromIndexed256(1);
        case Color::Basic::Green:         return Color::FromIndexed256(2);
        case Color::Basic::Yellow:        return Color::FromIndexed256(3);
        case Color::Basic::Blue:          return Color::FromIndexed256(4);
        case Color::Basic::Magenta:       return Color::FromIndexed256(5);
        case Color::Basic::Cyan:          return Color::FromIndexed256(6);
        case Color::Basic::White:         return Color::FromIndexed256(7);
        case Color::Basic::BrightBlack:   return Color::FromIndexed256(8);
        case Color::Basic::BrightRed:     return Color::FromIndexed256(9);
        case Color::Basic::BrightGreen:   return Color::FromIndexed256(10);
        case Color::Basic::BrightYellow:  return Color::FromIndexed256(11);
        case Color::Basic::BrightBlue:    return Color::FromIndexed256(12);
        case Color::Basic::BrightMagenta: return Color::FromIndexed256(13);
        case Color::Basic::BrightCyan:    return Color::FromIndexed256(14);
        case Color::Basic::BrightWhite:   return Color::FromIndexed256(15);
        default:                          return Color::FromIndexed256(7);
        }
    }

    const RgbColor rgb = toRgb(color);
    return Color::FromIndexed256(rgbToNearestIndexed256(rgb.red, rgb.green, rgb.blue));
}

StylePolicy::RgbColor StylePolicy::toRgb(const Color& color) const
{
    if (color.isRgb())
    {
        return { color.red(), color.green(), color.blue() };
    }

    if (color.isBasic())
    {
        return basicToRgb(color.basic());
    }

    if (color.isIndexed256())
    {
        const std::uint8_t index = color.index256();

        if (index < 16)
        {
            switch (index)
            {
            case 0:  return basicToRgb(Color::Basic::Black);
            case 1:  return basicToRgb(Color::Basic::Red);
            case 2:  return basicToRgb(Color::Basic::Green);
            case 3:  return basicToRgb(Color::Basic::Yellow);
            case 4:  return basicToRgb(Color::Basic::Blue);
            case 5:  return basicToRgb(Color::Basic::Magenta);
            case 6:  return basicToRgb(Color::Basic::Cyan);
            case 7:  return basicToRgb(Color::Basic::White);
            case 8:  return basicToRgb(Color::Basic::BrightBlack);
            case 9:  return basicToRgb(Color::Basic::BrightRed);
            case 10: return basicToRgb(Color::Basic::BrightGreen);
            case 11: return basicToRgb(Color::Basic::BrightYellow);
            case 12: return basicToRgb(Color::Basic::BrightBlue);
            case 13: return basicToRgb(Color::Basic::BrightMagenta);
            case 14: return basicToRgb(Color::Basic::BrightCyan);
            case 15: return basicToRgb(Color::Basic::BrightWhite);
            default: break;
            }
        }

        if (index >= 16 && index <= 231)
        {
            const int adjusted = static_cast<int>(index) - 16;
            const int r = adjusted / 36;
            const int g = (adjusted / 6) % 6;
            const int b = adjusted % 6;

            return {
                cubeValue(r),
                cubeValue(g),
                cubeValue(b)
            };
        }

        if (index >= 232)
        {
            const std::uint8_t gray = static_cast<std::uint8_t>(8 + (index - 232) * 10);
            return { gray, gray, gray };
        }
    }

    return { 0, 0, 0 };
}

StylePolicy::RgbColor StylePolicy::basicToRgb(Color::Basic color)
{
    switch (color)
    {
    case Color::Basic::Black:         return { 0, 0, 0 };
    case Color::Basic::Red:           return { 128, 0, 0 };
    case Color::Basic::Green:         return { 0, 128, 0 };
    case Color::Basic::Yellow:        return { 128, 128, 0 };
    case Color::Basic::Blue:          return { 0, 0, 128 };
    case Color::Basic::Magenta:       return { 128, 0, 128 };
    case Color::Basic::Cyan:          return { 0, 128, 128 };
    case Color::Basic::White:         return { 192, 192, 192 };

    case Color::Basic::BrightBlack:   return { 128, 128, 128 };
    case Color::Basic::BrightRed:     return { 255, 0, 0 };
    case Color::Basic::BrightGreen:   return { 0, 255, 0 };
    case Color::Basic::BrightYellow:  return { 255, 255, 0 };
    case Color::Basic::BrightBlue:    return { 0, 0, 255 };
    case Color::Basic::BrightMagenta: return { 255, 0, 255 };
    case Color::Basic::BrightCyan:    return { 0, 255, 255 };
    case Color::Basic::BrightWhite:   return { 255, 255, 255 };
    default:                          return { 192, 192, 192 };
    }
}

Color::Basic StylePolicy::rgbToNearestBasic(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue)
{
    struct Candidate
    {
        Color::Basic basic;
        RgbColor rgb;
    };

    static const Candidate candidates[] =
    {
        { Color::Basic::Black,         basicToRgb(Color::Basic::Black) },
        { Color::Basic::Red,           basicToRgb(Color::Basic::Red) },
        { Color::Basic::Green,         basicToRgb(Color::Basic::Green) },
        { Color::Basic::Yellow,        basicToRgb(Color::Basic::Yellow) },
        { Color::Basic::Blue,          basicToRgb(Color::Basic::Blue) },
        { Color::Basic::Magenta,       basicToRgb(Color::Basic::Magenta) },
        { Color::Basic::Cyan,          basicToRgb(Color::Basic::Cyan) },
        { Color::Basic::White,         basicToRgb(Color::Basic::White) },
        { Color::Basic::BrightBlack,   basicToRgb(Color::Basic::BrightBlack) },
        { Color::Basic::BrightRed,     basicToRgb(Color::Basic::BrightRed) },
        { Color::Basic::BrightGreen,   basicToRgb(Color::Basic::BrightGreen) },
        { Color::Basic::BrightYellow,  basicToRgb(Color::Basic::BrightYellow) },
        { Color::Basic::BrightBlue,    basicToRgb(Color::Basic::BrightBlue) },
        { Color::Basic::BrightMagenta, basicToRgb(Color::Basic::BrightMagenta) },
        { Color::Basic::BrightCyan,    basicToRgb(Color::Basic::BrightCyan) },
        { Color::Basic::BrightWhite,   basicToRgb(Color::Basic::BrightWhite) }
    };

    int bestDistance = -1;
    Color::Basic bestColor = Color::Basic::White;

    for (const Candidate& candidate : candidates)
    {
        const int distance = squareDistance(
            red, green, blue,
            candidate.rgb.red, candidate.rgb.green, candidate.rgb.blue);

        if (bestDistance < 0 || distance < bestDistance)
        {
            bestDistance = distance;
            bestColor = candidate.basic;
        }
    }

    return bestColor;
}

std::uint8_t StylePolicy::rgbToNearestIndexed256(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue)
{
    int bestDistance = -1;
    std::uint8_t bestIndex = 0;

    for (int i = 0; i < 256; ++i)
    {
        const Color indexedColor = Color::FromIndexed256(static_cast<std::uint8_t>(i));
        const StylePolicy policy;
        const RgbColor rgb = policy.toRgb(indexedColor);

        const int distance = squareDistance(
            red, green, blue,
            rgb.red, rgb.green, rgb.blue);

        if (bestDistance < 0 || distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = static_cast<std::uint8_t>(i);
        }
    }

    return bestIndex;
}
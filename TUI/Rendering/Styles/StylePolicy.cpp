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
    policy.m_trueColorColorMode = ColorRenderMode::DowngradeToBasic;

    policy.m_boldMode = TextAttributeRenderMode::Approximate;
    policy.m_dimMode = TextAttributeRenderMode::Approximate;
    policy.m_underlineMode = TextAttributeRenderMode::Approximate;
    policy.m_reverseMode = TextAttributeRenderMode::Approximate;
    policy.m_invisibleMode = TextAttributeRenderMode::Approximate;
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

ColorRenderMode StylePolicy::trueColorColorMode() const
{
    return m_trueColorColorMode;
}

ColorRenderMode StylePolicy::rgbColorMode() const
{
    return trueColorColorMode();
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

StylePolicy StylePolicy::withTrueColorColorMode(ColorRenderMode mode) const
{
    StylePolicy copy = *this;
    copy.m_trueColorColorMode = mode;
    return copy;
}

StylePolicy StylePolicy::withRgbColorMode(ColorRenderMode mode) const
{
    return withTrueColorColorMode(mode);
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

    if (color->isTrueColor())
    {
        return applyColorMode(*color, m_trueColorColorMode);
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

    const TrueColorValue trueColor = toTrueColor(color);
    return Color::FromBasic(rgbToNearestBasic(trueColor.red, trueColor.green, trueColor.blue));
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

    const TrueColorValue trueColor = toTrueColor(color);
    return Color::FromIndexed256(rgbToNearestIndexed256(trueColor.red, trueColor.green, trueColor.blue));
}

StylePolicy::TrueColorValue StylePolicy::toTrueColor(const Color& color) const
{
    if (color.isTrueColor())
    {
        return { color.red(), color.green(), color.blue() };
    }

    if (color.isBasic())
    {
        return basicToTrueColor(color.basic());
    }

    if (color.isIndexed256())
    {
        const std::uint8_t index = color.index256();

        if (index < 16)
        {
            switch (index)
            {
            case 0:  return basicToTrueColor(Color::Basic::Black);
            case 1:  return basicToTrueColor(Color::Basic::Red);
            case 2:  return basicToTrueColor(Color::Basic::Green);
            case 3:  return basicToTrueColor(Color::Basic::Yellow);
            case 4:  return basicToTrueColor(Color::Basic::Blue);
            case 5:  return basicToTrueColor(Color::Basic::Magenta);
            case 6:  return basicToTrueColor(Color::Basic::Cyan);
            case 7:  return basicToTrueColor(Color::Basic::White);
            case 8:  return basicToTrueColor(Color::Basic::BrightBlack);
            case 9:  return basicToTrueColor(Color::Basic::BrightRed);
            case 10: return basicToTrueColor(Color::Basic::BrightGreen);
            case 11: return basicToTrueColor(Color::Basic::BrightYellow);
            case 12: return basicToTrueColor(Color::Basic::BrightBlue);
            case 13: return basicToTrueColor(Color::Basic::BrightMagenta);
            case 14: return basicToTrueColor(Color::Basic::BrightCyan);
            case 15: return basicToTrueColor(Color::Basic::BrightWhite);
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

StylePolicy::TrueColorValue StylePolicy::basicToTrueColor(Color::Basic color)
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
        TrueColorValue trueColor;
    };

    static const Candidate candidates[] =
    {
        { Color::Basic::Black,         basicToTrueColor(Color::Basic::Black) },
        { Color::Basic::Red,           basicToTrueColor(Color::Basic::Red) },
        { Color::Basic::Green,         basicToTrueColor(Color::Basic::Green) },
        { Color::Basic::Yellow,        basicToTrueColor(Color::Basic::Yellow) },
        { Color::Basic::Blue,          basicToTrueColor(Color::Basic::Blue) },
        { Color::Basic::Magenta,       basicToTrueColor(Color::Basic::Magenta) },
        { Color::Basic::Cyan,          basicToTrueColor(Color::Basic::Cyan) },
        { Color::Basic::White,         basicToTrueColor(Color::Basic::White) },
        { Color::Basic::BrightBlack,   basicToTrueColor(Color::Basic::BrightBlack) },
        { Color::Basic::BrightRed,     basicToTrueColor(Color::Basic::BrightRed) },
        { Color::Basic::BrightGreen,   basicToTrueColor(Color::Basic::BrightGreen) },
        { Color::Basic::BrightYellow,  basicToTrueColor(Color::Basic::BrightYellow) },
        { Color::Basic::BrightBlue,    basicToTrueColor(Color::Basic::BrightBlue) },
        { Color::Basic::BrightMagenta, basicToTrueColor(Color::Basic::BrightMagenta) },
        { Color::Basic::BrightCyan,    basicToTrueColor(Color::Basic::BrightCyan) },
        { Color::Basic::BrightWhite,   basicToTrueColor(Color::Basic::BrightWhite) }
    };

    int bestDistance = -1;
    Color::Basic bestColor = Color::Basic::White;

    for (const Candidate& candidate : candidates)
    {
        const int distance = squareDistance(
            red, green, blue,
            candidate.trueColor.red, candidate.trueColor.green, candidate.trueColor.blue);

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
        const TrueColorValue trueColor = policy.toTrueColor(indexedColor);

        const int distance = squareDistance(
            red, green, blue,
            trueColor.red, trueColor.green, trueColor.blue);

        if (bestDistance < 0 || distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = static_cast<std::uint8_t>(i);
        }
    }

    return bestIndex;
}
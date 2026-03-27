#pragma once

#include <cstdint>
#include <optional>

#include "Rendering/Styles/Color.h"
#include "Rendering/Styles/Style.h"

enum class ColorRenderMode
{
    Direct,
    DowngradeToBasic,
    DowngradeToIndexed256,
    Omit
};

enum class TextAttributeRenderMode
{
    Direct,
    Omit
};

enum class BlinkRenderMode
{
    Direct,
    Omit,
    Emulate
};

struct ResolvedStyle
{
    Style presentedStyle;
    bool emulateSlowBlink = false;
    bool emulateFastBlink = false;
};

class StylePolicy
{
public:
    StylePolicy();

    static StylePolicy PreserveIntent();
    static StylePolicy BasicConsole();
    static StylePolicy BasicConsoleWithBlinkEmulation();

    ResolvedStyle resolve(const Style& style) const;

    ColorRenderMode basicColorMode() const;
    ColorRenderMode indexed256ColorMode() const;
    ColorRenderMode trueColorColorMode() const;
    ColorRenderMode rgbColorMode() const;

    TextAttributeRenderMode boldMode() const;
    TextAttributeRenderMode dimMode() const;
    TextAttributeRenderMode underlineMode() const;
    TextAttributeRenderMode reverseMode() const;
    TextAttributeRenderMode invisibleMode() const;
    TextAttributeRenderMode strikeMode() const;

    BlinkRenderMode slowBlinkMode() const;
    BlinkRenderMode fastBlinkMode() const;

    StylePolicy withBasicColorMode(ColorRenderMode mode) const;
    StylePolicy withIndexed256ColorMode(ColorRenderMode mode) const;
    StylePolicy withTrueColorColorMode(ColorRenderMode mode) const;
    StylePolicy withRgbColorMode(ColorRenderMode mode) const;

    StylePolicy withBoldMode(TextAttributeRenderMode mode) const;
    StylePolicy withDimMode(TextAttributeRenderMode mode) const;
    StylePolicy withUnderlineMode(TextAttributeRenderMode mode) const;
    StylePolicy withReverseMode(TextAttributeRenderMode mode) const;
    StylePolicy withInvisibleMode(TextAttributeRenderMode mode) const;
    StylePolicy withStrikeMode(TextAttributeRenderMode mode) const;

    StylePolicy withSlowBlinkMode(BlinkRenderMode mode) const;
    StylePolicy withFastBlinkMode(BlinkRenderMode mode) const;

private:
    struct TrueColorValue
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
    };

private:
    std::optional<Color> resolveColor(const std::optional<Color>& color) const;
    std::optional<Color> applyColorMode(const Color& color, ColorRenderMode mode) const;

    Color downgradeToBasic(const Color& color) const;
    Color downgradeToIndexed256(const Color& color) const;

    TrueColorValue toTrueColor(const Color& color) const;

    static TrueColorValue basicToTrueColor(Color::Basic color);
    static Color::Basic rgbToNearestBasic(std::uint8_t red, std::uint8_t green, std::uint8_t blue);
    static std::uint8_t rgbToNearestIndexed256(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

private:
    ColorRenderMode m_basicColorMode = ColorRenderMode::Direct;
    ColorRenderMode m_indexed256ColorMode = ColorRenderMode::Direct;
    ColorRenderMode m_trueColorColorMode = ColorRenderMode::Direct;

    TextAttributeRenderMode m_boldMode = TextAttributeRenderMode::Direct;
    TextAttributeRenderMode m_dimMode = TextAttributeRenderMode::Direct;
    TextAttributeRenderMode m_underlineMode = TextAttributeRenderMode::Direct;
    TextAttributeRenderMode m_reverseMode = TextAttributeRenderMode::Direct;
    TextAttributeRenderMode m_invisibleMode = TextAttributeRenderMode::Direct;
    TextAttributeRenderMode m_strikeMode = TextAttributeRenderMode::Direct;

    BlinkRenderMode m_slowBlinkMode = BlinkRenderMode::Direct;
    BlinkRenderMode m_fastBlinkMode = BlinkRenderMode::Direct;
};
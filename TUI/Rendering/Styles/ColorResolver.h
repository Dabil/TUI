#pragma once

#include <optional>

#include "Rendering/Capabilities/ColorSupport.h"
#include "Rendering/Styles/Color.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/ThemeColor.h"

class ColorResolver
{
public:
    /*
        Main entry point for authored color intent.

        Input:
            - Style::StyleColorValue
            - renderer-reported ColorSupport

        Output:
            - concrete Color suitable for the active backend tier
    */
    static Color resolve(
        const Style::StyleColorValue& authoredColor,
        ColorSupport support);

    /*
        Convenience overload for optional authored color values.
        Returns nullopt when the style field was not authored at all.
    */
    static std::optional<Color> resolve(
        const std::optional<Style::StyleColorValue>& authoredColor,
        ColorSupport support);

    static Color resolveConcreteColor(
        const Color& color,
        ColorSupport support);

    static Color resolveThemeColor(
        const ThemeColor& themeColor,
        ColorSupport support);

private:
    static Color resolveThemeColorForNone(const ThemeColor& themeColor);
    static Color resolveThemeColorForBasic16(const ThemeColor& themeColor);
    static Color resolveThemeColorForIndexed256(const ThemeColor& themeColor);
    static Color resolveThemeColorForRgb24(const ThemeColor& themeColor);
};
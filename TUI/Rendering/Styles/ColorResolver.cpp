#include "Rendering/Styles/ColorResolver.h"

#include "Rendering/Styles/ColorMapping.h"

Color ColorResolver::resolve(
    const Style::StyleColorValue& authoredColor,
    ColorSupport support)
{
    if (authoredColor.hasConcreteColor())
    {
        return resolveConcreteColor(*authoredColor.concreteColor(), support);
    }

    if (authoredColor.hasThemeColor())
    {
        return resolveThemeColor(*authoredColor.themeColor(), support);
    }

    return Color::Default();
}

std::optional<Color> ColorResolver::resolve(
    const std::optional<Style::StyleColorValue>& authoredColor,
    ColorSupport support)
{
    if (!authoredColor.has_value())
    {
        return std::nullopt;
    }

    return resolve(*authoredColor, support);
}

Color ColorResolver::resolveConcreteColor(
    const Color& color,
    ColorSupport support)
{
    return ColorMapping::mapToSupport(color, support);
}

Color ColorResolver::resolveThemeColor(
    const ThemeColor& themeColor,
    ColorSupport support)
{
    switch (support)
    {
    case ColorSupport::None:
        return resolveThemeColorForNone(themeColor);

    case ColorSupport::Basic16:
        return resolveThemeColorForBasic16(themeColor);

    case ColorSupport::Indexed256:
        return resolveThemeColorForIndexed256(themeColor);

    case ColorSupport::Rgb24:
        return resolveThemeColorForRgb24(themeColor);

    default:
        return resolveThemeColorForBasic16(themeColor);
    }
}

Color ColorResolver::resolveThemeColorForNone(const ThemeColor& themeColor)
{
    /*
        No backend color support means authored color intent cannot be presented.
        Returning Default keeps the result concrete while remaining policy-neutral.
    */
    (void)themeColor;
    return Color::Default();
}

Color ColorResolver::resolveThemeColorForBasic16(const ThemeColor& themeColor)
{
    /*
        Required resolution order:
            RGB -> Indexed -> Basic

        For a Basic16 backend:
            RGB      -> downgrade to Basic16
            Indexed  -> downgrade to Basic16
            Basic    -> direct
    */

    if (themeColor.hasRgb())
    {
        return ColorMapping::rgbToBasic(*themeColor.rgb());
    }

    if (themeColor.hasIndexed())
    {
        return ColorMapping::indexedToBasic(*themeColor.indexed());
    }

    if (themeColor.hasBasic())
    {
        return ColorMapping::mapToSupport(*themeColor.basic(), ColorSupport::Basic16);
    }

    return Color::Default();
}

Color ColorResolver::resolveThemeColorForIndexed256(const ThemeColor& themeColor)
{
    /*
        Required resolution order:
            RGB -> Indexed -> Basic

        For an Indexed256 backend:
            RGB      -> downgrade to Indexed256
            Indexed  -> direct
            Basic    -> direct
    */

    if (themeColor.hasRgb())
    {
        return ColorMapping::rgbToIndexed(*themeColor.rgb());
    }

    if (themeColor.hasIndexed())
    {
        return ColorMapping::mapToSupport(*themeColor.indexed(), ColorSupport::Indexed256);
    }

    if (themeColor.hasBasic())
    {
        return ColorMapping::mapToSupport(*themeColor.basic(), ColorSupport::Indexed256);
    }

    return Color::Default();
}

Color ColorResolver::resolveThemeColorForRgb24(const ThemeColor& themeColor)
{
    /*
        Required resolution order:
            RGB -> Indexed -> Basic

        For an Rgb24 backend:
            RGB      -> direct
            Indexed  -> direct
            Basic    -> direct

        Do not invent richer values here. Use the richest authored value that
        actually exists, but do not up-convert Basic/Indexed into synthetic RGB.
    */

    if (themeColor.hasRgb())
    {
        return *themeColor.rgb();
    }

    if (themeColor.hasIndexed())
    {
        return *themeColor.indexed();
    }

    if (themeColor.hasBasic())
    {
        return *themeColor.basic();
    }

    return Color::Default();
}
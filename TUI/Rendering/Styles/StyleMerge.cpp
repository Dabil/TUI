#include "Rendering/Styles/StyleMerge.h"

/*

Short integration note explaining how replace,
preserve, and merge should be used by callers

Use the modes like this:

Style result = StyleMerge::merge(existingStyle,
incomingStyle, StyleMergeMode::Replace);

Use Replace when:

an object or theme wants to fully define the
final style previous logical styling should be
discarded

Style result = StyleMerge::merge(existingStyle, incomingStyle, StyleMergeMode::PreserveDestination);

Use PreserveDestination when:

existing cell/style data must win completely
the incoming style is only conditional or fallback
Style result = StyleMerge::merge(existingStyle, incomingStyle, StyleMergeMode::MergePreserveDestination);

Use MergePreserveDestination when:

you want "apply this style on top"
color fields from the incoming style should
override only when present
boolean style fields should override only when
present so they can explicitly enable or disable
without erasing unspecified destination fields

This is the right logical merge behavior for
ScreenCell, ScreenBuffer, page composition, and
theme layering. Renderer downgrade or capability
handling should still happen later through
StylePolicy, not here.

*/

namespace
{
    void mergeOptionalColor(
        Style& result,
        const Style& source,
        bool foreground)
    {
        if (foreground)
        {
            if (source.hasForegroundColorValue())
            {
                if (source.hasForeground())
                {
                    result = result.withForeground(*source.foreground());
                }
                else if (source.hasForegroundThemeColor())
                {
                    result = result.withForeground(*source.foregroundThemeColor());
                }
            }
        }
        else
        {
            if (source.hasBackgroundColorValue())
            {
                if (source.hasBackground())
                {
                    result = result.withBackground(*source.background());
                }
                else if (source.hasBackgroundThemeColor())
                {
                    result = result.withBackground(*source.backgroundThemeColor());
                }
            }
        }
    }

    void mergeOptionalAttribute(
        Style& result,
        const Style::AttributeState& state,
        Style(Style::* setter)(bool) const)
    {
        if (state.has_value())
        {
            result = (result.*setter)(*state);
        }
    }
}

Style StyleMerge::merge(
    const Style& destination,
    const Style& source,
    StyleMergeMode mode)
{
    switch (mode)
    {
    case StyleMergeMode::Replace:
        return source;

    case StyleMergeMode::PreserveDestination:
        return destination;

    case StyleMergeMode::MergePreserveDestination:
        return mergePreserveDestination(destination, source);

    default:
        return destination;
    }
}

Style StyleMerge::mergePreserveDestination(
    const Style& destination,
    const Style& source)
{
    Style result = destination;

    mergeOptionalColor(result, source, true);
    mergeOptionalColor(result, source, false);

    mergeOptionalAttribute(result, source.boldState(), &Style::withBold);
    mergeOptionalAttribute(result, source.dimState(), &Style::withDim);
    mergeOptionalAttribute(result, source.underlineState(), &Style::withUnderline);
    mergeOptionalAttribute(result, source.slowBlinkState(), &Style::withSlowBlink);
    mergeOptionalAttribute(result, source.fastBlinkState(), &Style::withFastBlink);
    mergeOptionalAttribute(result, source.reverseState(), &Style::withReverse);
    mergeOptionalAttribute(result, source.invisibleState(), &Style::withInvisible);
    mergeOptionalAttribute(result, source.strikeState(), &Style::withStrike);

    return result;
}
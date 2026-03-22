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

you want “apply this style on top”
color fields from the incoming style should 
override only when present
boolean style flags should be added without 
erasing existing authored intent

This is the right logical merge behavior for 
ScreenCell, ScreenBuffer, page composition, and 
theme layering. Renderer downgrade or capability 
handling should still happen later through 
StylePolicy, not here.

*/

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

    if (source.hasForeground())
    {
        result = result.withForeground(*source.foreground());
    }

    if (source.hasBackground())
    {
        result = result.withBackground(*source.background());
    }

    if (source.bold())
    {
        result = result.withBold(true);
    }

    if (source.dim())
    {
        result = result.withDim(true);
    }

    if (source.underline())
    {
        result = result.withUnderline(true);
    }

    if (source.slowBlink())
    {
        result = result.withSlowBlink(true);
    }

    if (source.fastBlink())
    {
        result = result.withFastBlink(true);
    }

    if (source.reverse())
    {
        result = result.withReverse(true);
    }

    if (source.invisible())
    {
        result = result.withInvisible(true);
    }

    if (source.strike())
    {
        result = result.withStrike(true);
    }

    return result;
}
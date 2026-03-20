#include "Rendering/Styles/StyleMerge.h"

Style StyleMerge::merge(
    const Style& destination,
    const Style& source,
    StyleMergeMode mode)
{
    switch (mode)
    {
    case StyleMergeMode::Replace:
        return source;

    case StyleMergeMode::MergePreserveDestination:
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

    case StyleMergeMode::MergePreserveSource:
    {
        Style result = source;

        if (!source.hasForeground() && destination.hasForeground())
        {
            result = result.withForeground(*destination.foreground());
        }

        if (!source.hasBackground() && destination.hasBackground())
        {
            result = result.withBackground(*destination.background());
        }

        if (!source.bold() && destination.bold())
        {
            result = result.withBold(true);
        }

        if (!source.dim() && destination.dim())
        {
            result = result.withDim(true);
        }

        if (!source.underline() && destination.underline())
        {
            result = result.withUnderline(true);
        }

        if (!source.slowBlink() && destination.slowBlink())
        {
            result = result.withSlowBlink(true);
        }

        if (!source.fastBlink() && destination.fastBlink())
        {
            result = result.withFastBlink(true);
        }

        if (!source.reverse() && destination.reverse())
        {
            result = result.withReverse(true);
        }

        if (!source.invisible() && destination.invisible())
        {
            result = result.withInvisible(true);
        }

        if (!source.strike() && destination.strike())
        {
            result = result.withStrike(true);
        }

        return result;
    }

    default:
        return source;
    }
}
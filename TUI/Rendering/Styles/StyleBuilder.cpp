#include "Rendering/Styles/StyleBuilder.h"

namespace style
{
    const Style Bold = Style().withBold();
    const Style Dim = Style().withDim();
    const Style Underline = Style().withUnderline();
    const Style SlowBlink = Style().withSlowBlink();
    const Style FastBlink = Style().withFastBlink();
    const Style Reverse = Style().withReverse();
    const Style Invisible = Style().withInvisible();
    const Style Strike = Style().withStrike();

    Style Fg(const Color& color)
    {
        return Style().withForeground(color);
    }

    Style Bg(const Color& color)
    {
        return Style().withBackground(color);
    }
}

Style operator+(const Style& left, const Style& right)
{
    Style result = left;

    if (right.hasForeground())
    {
        result = result.withForeground(*right.foreground());
    }

    if (right.hasBackground())
    {
        result = result.withBackground(*right.background());
    }

    if (right.bold())
    {
        result = result.withBold(true);
    }

    if (right.dim())
    {
        result = result.withDim(true);
    }

    if (right.underline())
    {
        result = result.withUnderline(true);
    }

    if (right.slowBlink())
    {
        result = result.withSlowBlink(true);
    }

    if (right.fastBlink())
    {
        result = result.withFastBlink(true);
    }

    if (right.reverse())
    {
        result = result.withReverse(true);
    }

    if (right.invisible())
    {
        result = result.withInvisible(true);
    }

    if (right.strike())
    {
        result = result.withStrike(true);
    }

    return result;
}
#include "Rendering/Styles/StyleBuilder.h"

#include "Rendering/Styles/StyleMerge.h"

/*
Composition behavior with this design is simple and predictable:

left side is the base style
right side overlays onto it
foreground/background from the right replace the left if present
enabled boolean flags on the right are added to the result
this is still purely logical style composition, not renderer capability mapping

This makes it a good fit for themes, page composition, and any future semantic style namespace layer.
*/

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
    return StyleMerge::merge(left, right, StyleMergeMode::MergePreserveDestination);
}
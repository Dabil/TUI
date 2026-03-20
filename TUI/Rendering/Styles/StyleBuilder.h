#pragma once

#include "Rendering/Styles/Style.h"

namespace style
{
    extern const Style Bold;
    extern const Style Dim;
    extern const Style Underline;
    extern const Style SlowBlink;
    extern const Style FastBlink;
    extern const Style Reverse;
    extern const Style Invisible;
    extern const Style Strike;

    Style Fg(const Color& color);
    Style Bg(const Color& color);
}

// NOTE:
// operator+ must live in the same namespace as Style (global namespace),
// not inside namespace style, or expressions like:
//     style::Bold + style::Fg(...)
// will fail to resolve.
Style operator+(const Style& left, const Style& right);
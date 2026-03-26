#pragma once

#include "Rendering/Styles/Style.h"

namespace style
{
    struct BoldAtom
    {
        Style operator()() const
        {
            return Style().withBold();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    struct DimAtom
    {
        Style operator()() const
        {
            return Style().withDim();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    struct UnderlineAtom
    {
        Style operator()() const
        {
            return Style().withUnderline();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    struct SlowBlinkAtom
    {
        Style operator()() const
        {
            return Style().withSlowBlink();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    struct FastBlinkAtom
    {
        Style operator()() const
        {
            return Style().withFastBlink();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    struct ReverseAtom
    {
        Style operator()() const
        {
            return Style().withReverse();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    struct InvisibleAtom
    {
        Style operator()() const
        {
            return Style().withInvisible();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    struct StrikeAtom
    {
        Style operator()() const
        {
            return Style().withStrike();
        }

        operator Style() const
        {
            return operator()();
        }
    };

    inline constexpr BoldAtom Bold{};
    inline constexpr DimAtom Dim{};
    inline constexpr UnderlineAtom Underline{};
    inline constexpr SlowBlinkAtom SlowBlink{};
    inline constexpr FastBlinkAtom FastBlink{};
    inline constexpr ReverseAtom Reverse{};
    inline constexpr InvisibleAtom Invisible{};
    inline constexpr StrikeAtom Strike{};

    Style Fg(const Color& color);
    Style Bg(const Color& color);
}

// NOTE:
// operator+ must live in the same namespace as Style (global namespace),
// not inside namespace style, or expressions like:
//     style::Bold + style::Fg(...)
// will fail to resolve.
Style operator+(const Style& left, const Style& right);
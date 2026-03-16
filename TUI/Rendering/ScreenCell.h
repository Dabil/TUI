#pragma once

// Assuming future file
#include "Rendering/Style.h"

struct ScreenCell
{
    char32_t glyph = U' ';
    Style style{};

    bool hasVisibleGlyph() const
    {
        return glyph != U' ';
    }

    bool hasStyle() const
    {
        return style != Style{};
    }

    bool operator==(const ScreenCell& other) const
    {
        return glyph == other.glyph && style == other.style;
    }

    bool operator!=(const ScreenCell& other) const
    {
        return !(*this == other);
    }
};
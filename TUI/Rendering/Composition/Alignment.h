#pragma once

namespace Composition
{
    enum class HorizontalAlign
    {
        Left,
        Center,
        Right
    };

    enum class VerticalAlign
    {
        Top,
        Center,
        Bottom
    };

    struct Alignment
    {
        HorizontalAlign horizontal = HorizontalAlign::Left;
        VerticalAlign vertical = VerticalAlign::Top;
    };
}
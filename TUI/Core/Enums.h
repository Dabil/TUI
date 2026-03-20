#pragma once

/*
    TODO:

    Checklist:
        - Add a Unicode-aware frame style enum later if desired

    I don't want to make up enum's just to have them, so going to wait will 
    I have an actual need so I know what it will be.
*/

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

enum class Orientation
{
    Horizontal,
    Vertical
};

enum class Anchor
{
    TopLeft,
    TopCenter,
    TopRight,

    CenterLeft,
    Center,
    CenterRight,

    BottomLeft,
    BottomCenter,
    BottomRight
};
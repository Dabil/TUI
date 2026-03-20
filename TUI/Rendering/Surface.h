#pragma once

#include "Rendering/ScreenBuffer.h"

/*
    Update:

    Mostly no change. It is already a good wrapper around ScreenBuffeer

    Checklist:
        - No API change required
        - REbuild only if includes need adjustment
*/

class Surface
{
public:
    Surface();
    Surface(int width, int height);

    void resize(int width, int height);
    void clear(const Style& style);

    ScreenBuffer& buffer();
    const ScreenBuffer& buffer() const;

private:
    ScreenBuffer m_buffer;
};
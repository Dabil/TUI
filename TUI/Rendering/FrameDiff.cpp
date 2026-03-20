#include "Rendering/FrameDiff.h"
#include "Rendering/ScreenBuffer.h"

#include <stdexcept>

/*
    * This is what makes the console renderer fast and efficient *
     
    It figures out what changed between the last frame and this frame
    Instead of redrawing the entire screen every time, it only 
    redraws the cells that changed.

    FrameDiff compares previousFrame and currentFrame and finds differences

    The result is a list of "dirty spans":

    struct DirtySpan
    {
        int y;
        int xStart;
        int xEnd;
    }

    Meaning: Row y, from xStart to xEnd needs to be redrawn.

    Then we group changes into horizontal spans and have the 
    renderer update only what's necessary. 

    This results in a massive speed increase and reduction in 
    screen flicker. This matches how terminals work best.
*/

std::vector<DirtySpan> FrameDiff::diffRows(const ScreenBuffer& previous, const ScreenBuffer& current)
{
    if (previous.getWidth() != current.getWidth() ||
        previous.getHeight() != current.getHeight())
    {
        throw std::runtime_error("FrameDiff requires buffers of matching size.");
    }

    std::vector<DirtySpan> dirtySpans;

    const int width = current.getWidth();
    const int height = current.getHeight();

    for (int y = 0; y < height; ++y)
    {
        int firstDirty = -1;
        int lastDirty = -1;

        for (int x = 0; x < width; ++x)
        {
            if (previous.getCell(x, y) != current.getCell(x, y))
            {
                if (firstDirty == -1)
                {
                    firstDirty = x;
                }

                lastDirty = x;
            }
        }

        if (firstDirty != -1)
        {
            dirtySpans.push_back(DirtySpan{ y, firstDirty, lastDirty });
        }
    }

    return dirtySpans;
}
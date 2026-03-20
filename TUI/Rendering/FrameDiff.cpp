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

/*
* Update: 03/18/26
* 

FrameDiff compares the previous frame to the current frame and returns
only the exact horizontal spans that changed.

Example:
previous: ....XXX....YY....
current : ....AAA....ZZ....

Result :
    span 1 = [4..6]
    span 2 = [11..12]

    This is more efficient than returning one large span from first dirty
    cell to last dirty cell, because unchanged gaps are not redrawn.
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
        int x = 0;

        while (x < width)
        {
            // Skip unchanged cells until we find the start of a dirty run.
            while (x < width && previous.getCell(x, y) == current.getCell(x, y))
            {
                ++x;
            }

            if (x >= width)
            {
                break;
            }

            const int spanStart = x;

            // Advance until the dirty run ends.
            while (x < width && previous.getCell(x, y) != current.getCell(x, y))
            {
                ++x;
            }

            const int spanEnd = x - 1;

            dirtySpans.push_back(DirtySpan{ y, spanStart, spanEnd });
        }
    }

    return dirtySpans;
}
#pragma once

#include <string>

#include "Rendering/VtRun.h"
#include "Rendering/VtStyleState.h"

class VtFrameEmitter
{
public:
    explicit VtFrameEmitter(VtStyleState& styleState);

    void beginFrame(bool clearScreenFirst);
    void appendRun(const VtRun& run);
    std::string finishFrame(bool resetStyleAtEnd = false);

private:
    void appendCursorMove(int x, int y);

private:
    VtStyleState& m_styleState;
    std::string m_buffer;

    int m_cursorX = 0;
    int m_cursorY = 0;
    bool m_cursorKnown = false;
};
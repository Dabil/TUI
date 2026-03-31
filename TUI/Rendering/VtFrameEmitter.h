#pragma once

#include <string>

#include "Rendering/SgrEmitter.h"
#include "Rendering/VtRun.h"

class VtFrameEmitter
{
public:
    explicit VtFrameEmitter(SgrEmitter& sgrEmitter);

    void beginFrame(bool clearScreenFirst);
    void appendRun(const VtRun& run);
    std::string finishFrame(bool resetStyleAtEnd = false);

private:
    void appendCursorMove(int x, int y);

private:
    SgrEmitter& m_sgrEmitter;
    std::string m_buffer;

    int m_cursorX = 0;
    int m_cursorY = 0;
    bool m_cursorKnown = false;
};
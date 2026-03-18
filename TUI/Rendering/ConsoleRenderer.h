#pragma once

#include "Rendering/FrameDiff.h"
#include "Rendering/IRenderer.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Style.h"

#define NOMINMAX
#include <windows.h>

class ConsoleRenderer : public IRenderer
{
public:
    ConsoleRenderer();
    ~ConsoleRenderer() override;

    bool maximizeConsole();
    bool initialize() override;
    void shutdown() override;
    void present(const ScreenBuffer& frame) override;
    void resize(int width, int height) override;

    int getConsoleWidth() const;
    int getConsoleHeight() const;
    bool pollResize();

private:
    void writeFullFrame(const ScreenBuffer& frame);
    void writeDirtySpans(const ScreenBuffer& frame);

    void moveCursor(int x, int y);
    void setStyle(const Style& style);
    void resetStyle();
    void writeGlyph(char32_t glyph);

    bool queryVisibleConsoleSize(int& width, int& height) const;
    bool configureConsole();
    void restoreConsoleState();

private:
    HANDLE m_hOut = INVALID_HANDLE_VALUE;
    HANDLE m_hIn = INVALID_HANDLE_VALUE;

    int m_consoleWidth = 0;
    int m_consoleHeight = 0;

    ScreenBuffer m_previousFrame;
    bool m_initialized = false;
    bool m_firstPresent = true;

    Style m_currentStyle{};
    WORD m_defaultAttributes = 0;

    UINT m_originalOutputCodePage = 0;
    UINT m_originalInputCodePage = 0;
    DWORD m_originalOutputMode = 0;
    DWORD m_originalInputMode = 0;
    bool m_haveOriginalOutputMode = false;
    bool m_haveOriginalInputMode = false;
    bool m_cursorWasVisible = true;
};
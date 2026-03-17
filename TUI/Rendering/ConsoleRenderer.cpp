#include "ConsoleRenderer.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "Rendering/Styles/Color.h"

ConsoleRenderer::ConsoleRenderer() = default;

ConsoleRenderer::~ConsoleRenderer()
{
    shutdown();
}

bool ConsoleRenderer::initialize() override
{
    if (m_initialized)
    {
        return true;
    }

    m_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    m_hIn = GetStdHandle(STD_INPUT_HANDLE);

    if (m_hOut == INVALID_HANDLE_VALUE || m_hOut == nullptr)
    {
        return false;
    }

    if (m_hIn == INVALID_HANDLE_VALUE || m_hIn == nullptr)
    {
        return false;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(m_hOut, &info))
    {
        return false;
    }

    m_defaultAttributes = info.wAttributes;
    m_originalOutputCodePage = GetConsoleOutputCP();
    m_originalInputCodePage = GetConsoleCP();

    DWORD outMode = 0;
    if (GetConsoleMode(m_hOut, &outMode))
    {
        m_originalOutputMode = outMode;
        m_haveOriginalOutputMode = true;
    }

    DWORD inMode = 0;
    if (GetConsoleMode(m_hIn, &inMode))
    {
        m_originalInputMode = inMode;
        m_haveOriginalInputMode = true;
    }

    CONSOLE_CURSOR_INFO cursorInfo{};
    if (GetConsoleCursorInfo(m_hOut, &cursorInfo))
    {
        m_cursorWasVisible = (cursorInfo.bVisible != FALSE);
    }

    if (!configureConsole())
    {
        restoreConsoleState();
        return false;
    }

    if (!queryVisibleConsoleSize(m_consoleWidth, m_consoleHeight))
    {
        m_consoleWidth = 120;
        m_consoleHeight = 40;
    }

    m_previousFrame.resize(m_consoleWidth, m_consoleHeight);
    m_previousFrame.clear();

    m_currentStyle = Style{};
    m_firstPresent = true;
    m_initialized = true;

    return true;
}

void ConsoleRenderer::shutdown() override
{
    if (!m_initialized)
    {
        return;
    }

    resetStyle();
    restoreConsoleState();

    m_initialized = false;
    m_firstPresent = true;
    m_currentStyle = Style{};
}

void ConsoleRenderer::present(const ScreenBuffer& frame) override
{
    if (!m_initialized)
    {
        throw std::runtime_error("ConsoleRenderer::present called before initialize().");
    }

    if (m_firstPresent ||
        m_previousFrame.getWidth() != frame.getWidth() ||
        m_previousFrame.getHeight() != frame.getHeight())
    {
        m_previousFrame.resize(frame.getWidth(), frame.getHeight());
        m_previousFrame.clear();

        writeFullFrame(frame);

        m_previousFrame = frame;
        m_firstPresent = false;
        return;
    }

    writeDirtySpans(frame);
    m_previousFrame = frame;
}

void ConsoleRenderer::resize(int width, int height) override
{

}

int ConsoleRenderer::getConsoleWidth() const
{

}

int ConsoleRenderer::getConsoleHeight() const
{

}

bool ConsoleRenderer::pollResize()
{

}

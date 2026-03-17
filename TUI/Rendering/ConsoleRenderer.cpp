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
    m_consoleWidth = width;
    m_consoleHeight = height;

    m_previousFrame.resize(width, height);
    m_previousFrame.clear();

    m_firstPresent = true;
    m_currentStyle = Style{};
}

int ConsoleRenderer::getConsoleWidth() const
{
    return m_consoleWidth;
}

int ConsoleRenderer::getConsoleHeight() const
{
    return m_consoleHeight;
}

bool ConsoleRenderer::pollResize()
{
    int newWidth = 0;
    int newHeight = 0;

    if (!queryVisibleConsoleSize(newWidth, newHeight))
    {
        return false;
    }

    if (newWidth == m_consoleWidth && newHeight == m_consoleHeight)
    {
        return false;
    }

    resize(newWidth, newHeight);
    return true;
}

void ConsoleRenderer::writeFullFrame(const ScreenBuffer& frame)
{
    for (int y = 0; y < frame.getHeight(); ++y)
    {
        moveCursor(0, y);

        for (int x = 0; x < frame.getWidth(); ++x)
        {
            const ScreenCell& cell = frame.getCell(x, y);
            setStyle(cell.style);
            writeGlyph(cell.glyph);
        }
    }
}

void ConsoleRenderer::writeDirtySpans(const ScreenBuffer& frame)
{

}

void ConsoleRenderer::moveCursor(int x, int y)
{

}

void ConsoleRenderer::setStyle(const Style& style)
{

}

void ConsoleRenderer::resetStyle()
{

}

void ConsoleRenderer::writeGlyph(char32_t glyph)
{

}

bool ConsoleRenderer::queryVisibleConsoleSize(int& width, int& height) const
{

}

bool ConsoleRenderer::configureConsole()
{

}

void ConsoleRenderer::restoreConsoleState() 
{

}

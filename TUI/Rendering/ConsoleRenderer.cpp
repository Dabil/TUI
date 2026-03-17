#include "ConsoleRenderer.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "Rendering/Styles/Color.h"

namespace
{
    WORD basicForegroundBits(Color::Basic color)
    {
        switch (color)
        {
        case Color::Basic::Black:         return 0;
        case Color::Basic::Red:           return FOREGROUND_RED;
        case Color::Basic::Green:         return FOREGROUND_GREEN;
        case Color::Basic::Yellow:        return FOREGROUND_RED | FOREGROUND_GREEN;
        case Color::Basic::Blue:          return FOREGROUND_BLUE;
        case Color::Basic::Magenta:       return FOREGROUND_RED | FOREGROUND_BLUE;
        case Color::Basic::Cyan:          return FOREGROUND_GREEN | FOREGROUND_BLUE;
        case Color::Basic::White:         return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        case Color::Basic::BrightBlack:   return FOREGROUND_INTENSITY;
        case Color::Basic::BrightRed:     return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case Color::Basic::BrightGreen:   return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case Color::Basic::BrightYellow:  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case Color::Basic::BrightBlue:    return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Color::Basic::BrightMagenta: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Color::Basic::BrightCyan:    return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Color::Basic::BrightWhite:   return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        default:                          return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    }

    WORD basicBackgroundBits(Color::Basic color)
    {
        switch (color)
        {
        case Color::Basic::Black:         return 0;
        case Color::Basic::Red:           return BACKGROUND_RED;
        case Color::Basic::Green:         return BACKGROUND_GREEN;
        case Color::Basic::Yellow:        return BACKGROUND_RED | BACKGROUND_GREEN;
        case Color::Basic::Blue:          return BACKGROUND_BLUE;
        case Color::Basic::Magenta:       return BACKGROUND_RED | BACKGROUND_BLUE;
        case Color::Basic::Cyan:          return BACKGROUND_GREEN | BACKGROUND_BLUE;
        case Color::Basic::White:         return BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

        case Color::Basic::BrightBlack:   return BACKGROUND_INTENSITY;
        case Color::Basic::BrightRed:     return BACKGROUND_RED | BACKGROUND_INTENSITY;
        case Color::Basic::BrightGreen:   return BACKGROUND_GREEN | BACKGROUND_INTENSITY;
        case Color::Basic::BrightYellow:  return BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
        case Color::Basic::BrightBlue:    return BACKGROUND_BLUE | BACKGROUND_INTENSITY;
        case Color::Basic::BrightMagenta: return BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
        case Color::Basic::BrightCyan:    return BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
        case Color::Basic::BrightWhite:   return BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
        default:                          return 0;
        }
    }

    WORD swapForegroundToBackground(WORD fg)
    {
        WORD bg = 0;

        if (fg & FOREGROUND_RED)       bg |= BACKGROUND_RED;
        if (fg & FOREGROUND_GREEN)     bg |= BACKGROUND_GREEN;
        if (fg & FOREGROUND_BLUE)      bg |= BACKGROUND_BLUE;
        if (fg & FOREGROUND_INTENSITY) bg |= BACKGROUND_INTENSITY;

        return bg;
    }

    WORD swapBackgroundToForeground(WORD bg)
    {
        WORD fg = 0;

        if (bg & BACKGROUND_RED)       fg |= FOREGROUND_RED;
        if (bg & BACKGROUND_GREEN)     fg |= FOREGROUND_GREEN;
        if (bg & BACKGROUND_BLUE)      fg |= FOREGROUND_BLUE;
        if (bg & BACKGROUND_INTENSITY) fg |= FOREGROUND_INTENSITY;

        return fg;
    }

    WORD styleToAttributes(const Style& style, WORD defaultAttributes)
    {
        WORD attributes = defaultAttributes;

        if (style.hasForeground() && style.foreground()->isBasic())
        {
            attributes &= ~kForegroundMask;
            attributes |= basicForegroundBits(style.foreground()->basic());
        }

        if (style.hasBackground() && style.background()->isBasic())
        {
            attributes &= ~kBackgroundMask;
            attributes |= basicBackgroundBits(style.background()->basic());
        }

        if (style.bold())
        {
            attributes |= FOREGROUND_INTENSITY;
        }

        if (style.dim())
        {
            attributes &= ~FOREGROUND_INTENSITY;
        }

        if (style.reverse())
        {
            const WORD fg = attributes & kForegroundMask;
            const WORD bg = attributes & kBackgroundMask;

            attributes &= ~kForegroundMask;
            attributes &= ~kBackgroundMask;

            attributes |= swapBackgroundToForeground(bg);
            attributes |= swapForegroundToBackground(fg);

            attributes |= COMMON_LVB_REVERSE_VIDEO;
        }
        else
        {
            attributes &= ~COMMON_LVB_REVERSE_VIDEO;
        }

        if (style.underline())
        {
            attributes |= COMMON_LVB_UNDERSCORE;
        }
        else
        {
            attributes &= ~COMMON_LVB_UNDERSCORE;
        }

        if (style.invisible())
        {
            const WORD bg = attributes & kBackgroundMask;
            attributes &= ~kForegroundMask;
            attributes |= swapBackgroundToForeground(bg);
        }

        return attributes;
    }
 
    std::wstring codePointToUtf16(char32_t cp)
    {
        if (cp <= 0xFFFF)
        {
            return std::wstring(1, static_cast<wchar_t>(cp));
        }

        if (cp > 0x10FFFF)
        {
            return L"?";
        }

        cp -= 0x10000;
        const wchar_t high = static_cast<wchar_t>(0xD800 + ((cp >> 10) & 0x3FF));
        const wchar_t low = static_cast<wchar_t>(0xDC00 + (cp & 0x3FF));

        std::wstring result;
        result.push_back(high);
        result.push_back(low);
        return result;
    }

}

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
    const std::vector<DirtySpan> spans = FrameDiff::diffRows(m_previousFrame, frame);

    for (const DirtySpan& span : spans)
    {
        moveCursor(span.xStart, span.y);

        for (int x = span.xStart; x <= span.xEnd; ++x)
        {
            const ScreenCell& cell = frame.getCell(x, span.y);
            setStyle(cell.style);
            writeGlyph(cell.glyph);
        }
    }
}

void ConsoleRenderer::moveCursor(int x, int y)
{
    COORD pos{};
    pos.X = static_cast<SHORT>(x);
    pos.Y = static_cast<SHORT>(y);

    SetConsoleCursorPosition(m_hOut, pos);
}

void ConsoleRenderer::setStyle(const Style& style)
{
    if (style == m_currentStyle)
    {
        return;
    }

    const WORD attributes = styleToAttributes(style, m_defaultAttributes);
    SetConsoleTextAttribute(m_hOut, attributes);

    m_currentStyle = style;
}

void ConsoleRenderer::resetStyle()
{
    {
        SetConsoleTextAttribute(m_hOut, m_defaultAttributes);
        m_currentStyle = Style{};
    }
}

void ConsoleRenderer::writeGlyph(char32_t glyph)
{
    const std::wstring utf16 = codePointToUtf16(glyph);

    DWORD written = 0;
    WriteConsoleW(
        m_hOut,
        utf16.data(),
        static_cast<DWORD>(utf16.size()),
        &written,
        nullptr);
}

bool ConsoleRenderer::queryVisibleConsoleSize(int& width, int& height) const
{
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(m_hOut, &info))
    {
        return false;
    }

    width = static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
    height = static_cast<int>(info.srWindow.Bottom - info.srWindow.Top + 1);

    return width > 0 && height > 0;
}

bool ConsoleRenderer::configureConsole()
{

}

void ConsoleRenderer::restoreConsoleState() 
{

}

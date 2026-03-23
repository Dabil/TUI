#include "Rendering/ConsoleRenderer.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "Rendering/Styles/Color.h"
#include "Utilities/Unicode/UnicodeConversion.h"

/*
    ConsoleRenderer is now a backend/output layer.

    Checklist:
        - Remove local codePointToUtf16 ownership
        - Use UnicodeConversion::u32ToUtf16
        - Present only visible leading glyphs
        - Skip continuation glyph rendering
        - Group output into same-style spans

    Phase 2 Refactor:

    The flow after this refactor is:

    logical Style
        -> renderer StylePolicy resolution
        -> backend attribute mapping
        -> Win32 output

    So:

    Style stays rich and unchanged in ScreenCell / ScreenBuffer
    ConsoleRenderer uses StylePolicy to 
    downgrade or omit unsupported features only at presentation time
    the current console backend still renders through Win32 attributes, 
    but the policy layer now cleanly preserves future support for richer backends

    For future ConsoleCapabilities work, the clean upgrade path is:

    replace detectConsoleStyleCapabilities(...) with a 
    translation from your eventual shared ConsoleCapabilities type
    keep buildStylePolicy(...) as the single place that converts 
    backend support into renderer mapping behavior
    expose diagnostics by reporting:
    whether VT mode was enabled
    whether RGB/256 colors are direct or downgraded
    which style fields are direct vs omitted vs emulated

    That gives you a clean renderer-side architecture 
    without pushing backend assumptions back into authoring code.
*/

namespace
{
    constexpr WORD kForegroundMask =
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    constexpr WORD kBackgroundMask =
        BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;

    struct ConsoleStyleCapabilities
    {
        bool supportsBasicColors = true;
        bool supportsIndexed256Colors = false;
        bool supportsRgbColors = false;

        bool supportsBold = true;
        bool supportsDim = true;
        bool supportsUnderline = true;
        bool supportsReverse = true;
        bool supportsInvisible = true;
        bool supportsStrike = false;

        bool supportsSlowBlink = false;
        bool supportsFastBlink = false;
    };

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

    ConsoleStyleCapabilities detectConsoleStyleCapabilities(bool virtualTerminalEnabled)
    {
        ConsoleStyleCapabilities capabilities;

        capabilities.supportsBasicColors = true;
        capabilities.supportsIndexed256Colors = false;
        capabilities.supportsRgbColors = false;

        capabilities.supportsBold = true;
        capabilities.supportsDim = true;
        capabilities.supportsUnderline = true;
        capabilities.supportsReverse = true;
        capabilities.supportsInvisible = true;
        capabilities.supportsStrike = false;

        capabilities.supportsSlowBlink = false;
        capabilities.supportsFastBlink = false;

        /*
            For the current renderer implementation we still use Win32 attribute
            mapping through SetConsoleTextAttribute, not ANSI transport strings.

            Even if VT processing is enabled, the logical style mapping should
            remain conservative until an alternate VT output path is introduced.
        */
        (void)virtualTerminalEnabled;

        return capabilities;
    }

    StylePolicy buildStylePolicy(const ConsoleStyleCapabilities& capabilities)
    {
        StylePolicy policy = StylePolicy::PreserveIntent();

        policy = policy.withBasicColorMode(
            capabilities.supportsBasicColors
            ? ColorRenderMode::Direct
            : ColorRenderMode::Omit);

        policy = policy.withIndexed256ColorMode(
            capabilities.supportsIndexed256Colors
            ? ColorRenderMode::Direct
            : (capabilities.supportsBasicColors
                ? ColorRenderMode::DowngradeToBasic
                : ColorRenderMode::Omit));

        policy = policy.withRgbColorMode(
            capabilities.supportsRgbColors
            ? ColorRenderMode::Direct
            : (capabilities.supportsIndexed256Colors
                ? ColorRenderMode::DowngradeToIndexed256
                : (capabilities.supportsBasicColors
                    ? ColorRenderMode::DowngradeToBasic
                    : ColorRenderMode::Omit)));

        policy = policy.withBoldMode(
            capabilities.supportsBold
            ? TextAttributeRenderMode::Direct
            : TextAttributeRenderMode::Omit);

        policy = policy.withDimMode(
            capabilities.supportsDim
            ? TextAttributeRenderMode::Direct
            : TextAttributeRenderMode::Omit);

        policy = policy.withUnderlineMode(
            capabilities.supportsUnderline
            ? TextAttributeRenderMode::Direct
            : TextAttributeRenderMode::Omit);

        policy = policy.withReverseMode(
            capabilities.supportsReverse
            ? TextAttributeRenderMode::Direct
            : TextAttributeRenderMode::Omit);

        policy = policy.withInvisibleMode(
            capabilities.supportsInvisible
            ? TextAttributeRenderMode::Direct
            : TextAttributeRenderMode::Omit);

        policy = policy.withStrikeMode(
            capabilities.supportsStrike
            ? TextAttributeRenderMode::Direct
            : TextAttributeRenderMode::Omit);

        policy = policy.withSlowBlinkMode(
            capabilities.supportsSlowBlink
            ? BlinkRenderMode::Direct
            : BlinkRenderMode::Omit);

        policy = policy.withFastBlinkMode(
            capabilities.supportsFastBlink
            ? BlinkRenderMode::Direct
            : BlinkRenderMode::Omit);

        return policy;
    }

    WORD resolvedStyleToAttributes(const Style& style, WORD defaultAttributes)
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

    bool isContinuationCell(const ScreenCell& cell)
    {
        return cell.kind == CellKind::WideTrailing ||
            cell.kind == CellKind::CombiningContinuation;
    }

    char32_t cellToPresentedGlyph(const ScreenCell& cell)
    {
        if (isContinuationCell(cell))
        {
            return U' ';
        }

        if (cell.kind == CellKind::Glyph)
        {
            return cell.glyph;
        }

        return U' ';
    }
}

ConsoleRenderer::ConsoleRenderer() = default;

ConsoleRenderer::~ConsoleRenderer()
{
    shutdown();
}

bool ConsoleRenderer::maximizeConsole()
{
    if (m_hOut == INVALID_HANDLE_VALUE || m_hOut == nullptr)
    {
        return false;
    }

    COORD largestSize = GetLargestConsoleWindowSize(m_hOut);
    if (largestSize.X == 0 || largestSize.Y == 0)
    {
        return false;
    }

    COORD bufferSize{};
    bufferSize.X = largestSize.X;
    bufferSize.Y = largestSize.Y;

    if (!SetConsoleScreenBufferSize(m_hOut, bufferSize))
    {
        return false;
    }

    SMALL_RECT windowRect{};
    windowRect.Left = 0;
    windowRect.Top = 0;
    windowRect.Right = largestSize.X - 1;
    windowRect.Bottom = largestSize.Y - 1;

    if (!SetConsoleWindowInfo(m_hOut, TRUE, &windowRect))
    {
        return false;
    }

    HWND hwnd = GetConsoleWindow();
    if (hwnd != nullptr)
    {
        ShowWindow(hwnd, SW_MAXIMIZE);
    }

    return true;
}

bool ConsoleRenderer::initialize()
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

    maximizeConsole();

    if (!queryVisibleConsoleSize(m_consoleWidth, m_consoleHeight))
    {
        m_consoleWidth = 120;
        m_consoleHeight = 40;
    }

    m_previousFrame.resize(m_consoleWidth, m_consoleHeight);
    m_previousFrame.clear();

    StylePolicy policy = StylePolicy::PreserveIntent();

    policy = policy.withBasicColorMode(
        m_capabilities.supportsBasicColors()
        ? ColorRenderMode::Direct
        : ColorRenderMode::Omit);

    policy = policy.withIndexed256ColorMode(
        m_capabilities.supportsIndexed256Colors()
        ? ColorRenderMode::Direct
        : (m_capabilities.supportsBasicColors()
            ? ColorRenderMode::DowngradeToBasic
            : ColorRenderMode::Omit));

    policy = policy.withRgbColorMode(
        m_capabilities.supportsTrueColor()
        ? ColorRenderMode::Direct
        : (m_capabilities.supportsIndexed256Colors()
            ? ColorRenderMode::DowngradeToIndexed256
            : (m_capabilities.supportsBasicColors()
                ? ColorRenderMode::DowngradeToBasic
                : ColorRenderMode::Omit)));

    policy = policy.withBoldMode(
        m_capabilities.supportsBoldDirect()
        ? TextAttributeRenderMode::Direct
        : TextAttributeRenderMode::Omit);

    policy = policy.withDimMode(
        m_capabilities.supportsDimDirect()
        ? TextAttributeRenderMode::Direct
        : TextAttributeRenderMode::Omit);

    policy = policy.withUnderlineMode(
        m_capabilities.supportsUnderlineDirect()
        ? TextAttributeRenderMode::Direct
        : TextAttributeRenderMode::Omit);

    policy = policy.withReverseMode(
        m_capabilities.supportsReverseDirect()
        ? TextAttributeRenderMode::Direct
        : TextAttributeRenderMode::Omit);

    policy = policy.withInvisibleMode(
        m_capabilities.supportsInvisibleDirect()
        ? TextAttributeRenderMode::Direct
        : TextAttributeRenderMode::Omit);

    policy = policy.withStrikeMode(
        m_capabilities.supportsStrikeDirect()
        ? TextAttributeRenderMode::Direct
        : TextAttributeRenderMode::Omit);

    policy = policy.withSlowBlinkMode(
        m_capabilities.supportsSlowBlinkDirect()
        ? BlinkRenderMode::Direct
        : (m_capabilities.mayEmulateSlowBlink()
            ? BlinkRenderMode::Emulate
            : BlinkRenderMode::Omit));

    policy = policy.withFastBlinkMode(
        m_capabilities.supportsFastBlinkDirect()
        ? BlinkRenderMode::Direct
        : (m_capabilities.mayEmulateFastBlink()
            ? BlinkRenderMode::Emulate
            : BlinkRenderMode::Omit));

    m_stylePolicy = policy;

    m_currentStyle = Style{};
    m_firstPresent = true;
    m_initialized = true;

    return true;
}

void ConsoleRenderer::shutdown()
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

void ConsoleRenderer::present(const ScreenBuffer& frame)
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

void ConsoleRenderer::resize(int width, int height)
{
    m_consoleWidth = width;
    m_consoleHeight = height;

    m_previousFrame.resize(width, height);
    m_previousFrame.clear();

    m_firstPresent = true;
    m_currentStyle = Style{};
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

int ConsoleRenderer::getConsoleWidth() const
{
    return m_consoleWidth;
}

int ConsoleRenderer::getConsoleHeight() const
{
    return m_consoleHeight;
}

TextBackendCapabilities ConsoleRenderer::textCapabilities() const
{
    TextBackendCapabilities capabilities;
    capabilities.supportsUtf16Output = true;
    capabilities.supportsCombiningMarks = false;
    capabilities.supportsEastAsianWide = true;
    capabilities.supportsEmoji = false;
    capabilities.supportsFontFallback = false;
    return capabilities;
}

void ConsoleRenderer::writeFullFrame(const ScreenBuffer& frame)
{
    for (int y = 0; y < frame.getHeight(); ++y)
    {
        writeSpan(frame, y, 0, frame.getWidth() - 1);
    }
}

void ConsoleRenderer::writeDirtySpans(const ScreenBuffer& frame)
{
    const std::vector<DirtySpan> spans = FrameDiff::diffRows(m_previousFrame, frame);

    for (const DirtySpan& span : spans)
    {
        writeSpan(frame, span.y, span.xStart, span.xEnd);
    }
}

void ConsoleRenderer::writeSpan(const ScreenBuffer& frame, int y, int xStart, int xEnd)
{
    if (y < 0 || y >= frame.getHeight())
    {
        return;
    }

    if (frame.getWidth() <= 0)
    {
        return;
    }

    if (xStart < 0)
    {
        xStart = 0;
    }

    if (xEnd >= frame.getWidth())
    {
        xEnd = frame.getWidth() - 1;
    }

    if (xStart > xEnd)
    {
        return;
    }

    int x = xStart;

    while (x <= xEnd)
    {
        const ScreenCell& firstCell = frame.getCell(x, y);
        const Style runStyle = firstCell.style;
        const int runStart = x;

        std::u32string runText;
        runText.reserve(static_cast<std::size_t>(xEnd - runStart + 1));

        while (x <= xEnd)
        {
            const ScreenCell& cell = frame.getCell(x, y);
            if (cell.style != runStyle)
            {
                break;
            }

            runText.push_back(cellToPresentedGlyph(cell));
            ++x;
        }

        moveCursor(runStart, y);
        setStyle(runStyle);

        if (!runText.empty())
        {
            const std::wstring utf16 = UnicodeConversion::u32ToUtf16(runText);

            if (!utf16.empty())
            {
                DWORD written = 0;
                WriteConsoleW(
                    m_hOut,
                    utf16.data(),
                    static_cast<DWORD>(utf16.size()),
                    &written,
                    nullptr);
            }
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
    const ResolvedStyle resolved = m_stylePolicy.resolve(style);
    const Style& presentedStyle = resolved.presentedStyle;

    if (presentedStyle == m_currentStyle)
    {
        return;
    }

    const WORD attributes = resolvedStyleToAttributes(presentedStyle, m_defaultAttributes);
    SetConsoleTextAttribute(m_hOut, attributes);

    m_currentStyle = presentedStyle;
}

void ConsoleRenderer::resetStyle()
{
    SetConsoleTextAttribute(m_hOut, m_defaultAttributes);
    m_currentStyle = Style{};
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
    if (!SetConsoleOutputCP(CP_UTF8))
    {
        return false;
    }

    if (!SetConsoleCP(CP_UTF8))
    {
        return false;
    }

    const ConsoleCapabilityDetectionResult detection =
        ConsoleCapabilityDetector::detectAndConfigure(
            m_hOut,
            m_hIn,
            m_originalOutputMode,
            m_originalInputMode,
            m_haveOriginalOutputMode,
            m_haveOriginalInputMode);

    if (m_haveOriginalOutputMode && !detection.hasConfiguredOutputMode)
    {
        return false;
    }

    if (m_haveOriginalInputMode && !detection.hasConfiguredInputMode)
    {
        return false;
    }

    m_capabilities = detection.capabilities;
    m_virtualTerminalEnabled = detection.virtualTerminalWasEnabled;

    CONSOLE_CURSOR_INFO cursorInfo{};
    if (GetConsoleCursorInfo(m_hOut, &cursorInfo))
    {
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(m_hOut, &cursorInfo);
    }

    return true;
}

void ConsoleRenderer::restoreConsoleState()
{
    if (m_hOut != INVALID_HANDLE_VALUE && m_hOut != nullptr)
    {
        SetConsoleTextAttribute(m_hOut, m_defaultAttributes);

        if (m_haveOriginalOutputMode)
        {
            SetConsoleMode(m_hOut, m_originalOutputMode);
        }

        CONSOLE_CURSOR_INFO cursorInfo{};
        if (GetConsoleCursorInfo(m_hOut, &cursorInfo))
        {
            cursorInfo.bVisible = m_cursorWasVisible ? TRUE : FALSE;
            SetConsoleCursorInfo(m_hOut, &cursorInfo);
        }
    }

    if (m_hIn != INVALID_HANDLE_VALUE && m_hIn != nullptr)
    {
        if (m_haveOriginalInputMode)
        {
            SetConsoleMode(m_hIn, m_originalInputMode);
        }
    }

    if (m_originalOutputCodePage != 0)
    {
        SetConsoleOutputCP(m_originalOutputCodePage);
    }

    if (m_originalInputCodePage != 0)
    {
        SetConsoleCP(m_originalInputCodePage);
    }
}
#include "Rendering/TerminalRenderer.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Rendering/Diagnostics/RenderDiagnosticsWriter.h"
#include "Utilities/Unicode/UnicodeConversion.h"
#include "Utilities/Unicode/UnicodeWidth.h"

#include "Rendering/VtFrameEmitter.h"
#include "Rendering/VtRun.h"
#include "Rendering/VtStyleState.h"

/*
* New Frame emission flow
* 
ScreenBuffer
  -> FrameDiff::diffRows(previous, current)
  -> dirty spans per row
  -> ScreenBuffer::expandSpanToGlyphBoundaries(...)
  -> TerminalRenderer::buildRun(...)
       - skip continuation cells
       - group contiguous cells by authored style
       - resolve style once per run
       - convert run glyphs to UTF-8
  -> VtFrameEmitter::appendRun(...)
       - move cursor only when needed
       - transition VT style only when needed
       - append UTF-8 payload into one buffer
  -> TerminalRenderer::writeRaw(one buffered VT string)
*/

namespace
{
    constexpr std::size_t kMaxRecordedExamples = 24;
    constexpr std::size_t kMaxExamplesPerFeatureKind = 3;

    TextAttributeRenderMode textAttributeModeFromSupport(ConsoleFeatureSupport support)
    {
        switch (support)
        {
        case ConsoleFeatureSupport::Supported:
            return TextAttributeRenderMode::Direct;

        case ConsoleFeatureSupport::Unknown:
            return TextAttributeRenderMode::Approximate;

        case ConsoleFeatureSupport::Emulated:
        case ConsoleFeatureSupport::Unsupported:
        default:
            return TextAttributeRenderMode::Omit;
        }
    }

    StylePolicy buildStylePolicyFromCapabilities(const ConsoleCapabilities& capabilities)
    {
        StylePolicy policy = StylePolicy::PreserveIntent();

        policy = policy.withBasicColorMode(
            capabilities.supportsBasicColors()
            ? ColorRenderMode::Direct
            : ColorRenderMode::Omit);

        policy = policy.withIndexed256ColorMode(
            capabilities.supportsIndexed256Colors()
            ? ColorRenderMode::Direct
            : (capabilities.supportsBasicColors()
                ? ColorRenderMode::DowngradeToBasic
                : ColorRenderMode::Omit));

        policy = policy.withRgbColorMode(
            capabilities.supportsTrueColor()
            ? ColorRenderMode::Direct
            : (capabilities.supportsIndexed256Colors()
                ? ColorRenderMode::DowngradeToIndexed256
                : (capabilities.supportsBasicColors()
                    ? ColorRenderMode::DowngradeToBasic
                    : ColorRenderMode::Omit)));

        policy = policy.withBoldMode(textAttributeModeFromSupport(capabilities.bold));
        policy = policy.withDimMode(textAttributeModeFromSupport(capabilities.dim));
        policy = policy.withUnderlineMode(textAttributeModeFromSupport(capabilities.underline));
        policy = policy.withReverseMode(textAttributeModeFromSupport(capabilities.reverse));
        policy = policy.withInvisibleMode(textAttributeModeFromSupport(capabilities.invisible));
        policy = policy.withStrikeMode(textAttributeModeFromSupport(capabilities.strike));

        const bool allowSafeFallback = capabilities.usesPreserveStyleSafeFallback();

        policy = policy.withSlowBlinkMode(
            capabilities.supportsSlowBlinkDirect()
            ? BlinkRenderMode::Direct
            : ((allowSafeFallback && capabilities.mayEmulateSlowBlink())
                ? BlinkRenderMode::Emulate
                : BlinkRenderMode::Omit));

        policy = policy.withFastBlinkMode(
            capabilities.supportsFastBlinkDirect()
            ? BlinkRenderMode::Direct
            : ((allowSafeFallback && capabilities.mayEmulateFastBlink())
                ? BlinkRenderMode::Emulate
                : BlinkRenderMode::Omit));

        return policy;
    }

    bool isContinuationCell(const ScreenCell& cell)
    {
        return cell.kind == CellKind::WideTrailing ||
            cell.kind == CellKind::CombiningContinuation;
    }

    CellWidth presentedGlyphWidth(char32_t glyph)
    {
        if (glyph == U'\0')
        {
            return CellWidth::Zero;
        }

        if (glyph == U' ')
        {
            return CellWidth::One;
        }

        return UnicodeWidth::measureCodePointWidth(glyph);
    }

    char32_t cellToPresentedGlyph(const ScreenCell& cell)
    {
        if (isContinuationCell(cell))
        {
            return U'\0';
        }

        if (cell.kind == CellKind::Glyph)
        {
            return cell.glyph;
        }

        return U' ';
    }

    int glyphCellAdvance(const ScreenCell& cell)
    {
        if (cell.kind == CellKind::Glyph && cell.width == CellWidth::Two)
        {
            return 2;
        }

        if (isContinuationCell(cell))
        {
            return 0;
        }

        return 1;
    }

    void appendSgrCode(std::string& sequence, int code)
    {
        sequence += ';';
        sequence += std::to_string(code);
    }

    void appendBasicForeground(std::string& sequence, Color::Basic color)
    {
        switch (color)
        {
        case Color::Basic::Black:         appendSgrCode(sequence, 30); break;
        case Color::Basic::Red:           appendSgrCode(sequence, 31); break;
        case Color::Basic::Green:         appendSgrCode(sequence, 32); break;
        case Color::Basic::Yellow:        appendSgrCode(sequence, 33); break;
        case Color::Basic::Blue:          appendSgrCode(sequence, 34); break;
        case Color::Basic::Magenta:       appendSgrCode(sequence, 35); break;
        case Color::Basic::Cyan:          appendSgrCode(sequence, 36); break;
        case Color::Basic::White:         appendSgrCode(sequence, 37); break;
        case Color::Basic::BrightBlack:   appendSgrCode(sequence, 90); break;
        case Color::Basic::BrightRed:     appendSgrCode(sequence, 91); break;
        case Color::Basic::BrightGreen:   appendSgrCode(sequence, 92); break;
        case Color::Basic::BrightYellow:  appendSgrCode(sequence, 93); break;
        case Color::Basic::BrightBlue:    appendSgrCode(sequence, 94); break;
        case Color::Basic::BrightMagenta: appendSgrCode(sequence, 95); break;
        case Color::Basic::BrightCyan:    appendSgrCode(sequence, 96); break;
        case Color::Basic::BrightWhite:   appendSgrCode(sequence, 97); break;
        }
    }

    void appendBasicBackground(std::string& sequence, Color::Basic color)
    {
        switch (color)
        {
        case Color::Basic::Black:         appendSgrCode(sequence, 40); break;
        case Color::Basic::Red:           appendSgrCode(sequence, 41); break;
        case Color::Basic::Green:         appendSgrCode(sequence, 42); break;
        case Color::Basic::Yellow:        appendSgrCode(sequence, 43); break;
        case Color::Basic::Blue:          appendSgrCode(sequence, 44); break;
        case Color::Basic::Magenta:       appendSgrCode(sequence, 45); break;
        case Color::Basic::Cyan:          appendSgrCode(sequence, 46); break;
        case Color::Basic::White:         appendSgrCode(sequence, 47); break;
        case Color::Basic::BrightBlack:   appendSgrCode(sequence, 100); break;
        case Color::Basic::BrightRed:     appendSgrCode(sequence, 101); break;
        case Color::Basic::BrightGreen:   appendSgrCode(sequence, 102); break;
        case Color::Basic::BrightYellow:  appendSgrCode(sequence, 103); break;
        case Color::Basic::BrightBlue:    appendSgrCode(sequence, 104); break;
        case Color::Basic::BrightMagenta: appendSgrCode(sequence, 105); break;
        case Color::Basic::BrightCyan:    appendSgrCode(sequence, 106); break;
        case Color::Basic::BrightWhite:   appendSgrCode(sequence, 107); break;
        }
    }

    void appendForegroundColor(std::string& sequence, const Color& color)
    {
        if (color.isDefault())
        {
            appendSgrCode(sequence, 39);
            return;
        }

        if (color.isBasic())
        {
            appendBasicForeground(sequence, color.basic());
            return;
        }

        if (color.isIndexed256())
        {
            sequence += ";38;5;";
            sequence += std::to_string(static_cast<int>(color.index256()));
            return;
        }

        if (color.isTrueColor())
        {
            sequence += ";38;2;";
            sequence += std::to_string(static_cast<int>(color.red()));
            sequence += ';';
            sequence += std::to_string(static_cast<int>(color.green()));
            sequence += ';';
            sequence += std::to_string(static_cast<int>(color.blue()));
        }
    }

    void appendBackgroundColor(std::string& sequence, const Color& color)
    {
        if (color.isDefault())
        {
            appendSgrCode(sequence, 49);
            return;
        }

        if (color.isBasic())
        {
            appendBasicBackground(sequence, color.basic());
            return;
        }

        if (color.isIndexed256())
        {
            sequence += ";48;5;";
            sequence += std::to_string(static_cast<int>(color.index256()));
            return;
        }

        if (color.isTrueColor())
        {
            sequence += ";48;2;";
            sequence += std::to_string(static_cast<int>(color.red()));
            sequence += ';';
            sequence += std::to_string(static_cast<int>(color.green()));
            sequence += ';';
            sequence += std::to_string(static_cast<int>(color.blue()));
        }
    }

    std::string styleToAnsiSequence(const Style& style)
    {
        std::string sequence = "\x1b[0";

        if (style.hasForeground())
        {
            appendForegroundColor(sequence, *style.foreground());
        }

        if (style.hasBackground())
        {
            appendBackgroundColor(sequence, *style.background());
        }

        if (style.bold())
        {
            appendSgrCode(sequence, 1);
        }

        if (style.dim())
        {
            appendSgrCode(sequence, 2);
        }

        if (style.underline())
        {
            appendSgrCode(sequence, 4);
        }

        if (style.slowBlink())
        {
            appendSgrCode(sequence, 5);
        }

        if (style.fastBlink())
        {
            appendSgrCode(sequence, 6);
        }

        if (style.reverse())
        {
            appendSgrCode(sequence, 7);
        }

        if (style.invisible())
        {
            appendSgrCode(sequence, 8);
        }

        if (style.strike())
        {
            appendSgrCode(sequence, 9);
        }

        sequence += 'm';
        return sequence;
    }

    const char* basicColorToString(Color::Basic color)
    {
        switch (color)
        {
        case Color::Basic::Black:         return "Black";
        case Color::Basic::Red:           return "Red";
        case Color::Basic::Green:         return "Green";
        case Color::Basic::Yellow:        return "Yellow";
        case Color::Basic::Blue:          return "Blue";
        case Color::Basic::Magenta:       return "Magenta";
        case Color::Basic::Cyan:          return "Cyan";
        case Color::Basic::White:         return "White";
        case Color::Basic::BrightBlack:   return "BrightBlack";
        case Color::Basic::BrightRed:     return "BrightRed";
        case Color::Basic::BrightGreen:   return "BrightGreen";
        case Color::Basic::BrightYellow:  return "BrightYellow";
        case Color::Basic::BrightBlue:    return "BrightBlue";
        case Color::Basic::BrightMagenta: return "BrightMagenta";
        case Color::Basic::BrightCyan:    return "BrightCyan";
        case Color::Basic::BrightWhite:   return "BrightWhite";
        default:                          return "UnknownBasic";
        }
    }

    std::string colorToString(const std::optional<Color>& color)
    {
        if (!color.has_value())
        {
            return "None";
        }

        if (color->isDefault())
        {
            return "Default";
        }

        std::ostringstream stream;

        if (color->isBasic())
        {
            stream << "Basic(" << basicColorToString(color->basic()) << ")";
            return stream.str();
        }

        if (color->isIndexed256())
        {
            stream << "Indexed256(" << static_cast<int>(color->index256()) << ")";
            return stream.str();
        }

        if (color->isTrueColor())
        {
            stream
                << "TrueColor("
                << static_cast<int>(color->red()) << ","
                << static_cast<int>(color->green()) << ","
                << static_cast<int>(color->blue()) << ")";
            return stream.str();
        }

        return "UnknownColor";
    }

    bool rendererPhysicallyRendersColor(const std::optional<Color>& color)
    {
        return color.has_value();
    }

    LogicalStyleValueState logicalStateFromAttributeState(const Style::AttributeState& state)
    {
        if (!state.has_value())
        {
            return LogicalStyleValueState::Unspecified;
        }

        return *state
            ? LogicalStyleValueState::ExplicitlyEnabled
            : LogicalStyleValueState::ExplicitlyDisabled;
    }

    bool shouldCaptureExample(const CapabilityReport& report, StyleFeature feature, StyleAdaptationKind kind)
    {
        return report.examples().size() < kMaxRecordedExamples &&
            report.getCount(feature, kind) <= kMaxExamplesPerFeatureKind;
    }

    bool shouldCaptureLogicalStateExample(
        const CapabilityReport& report,
        StyleFeature feature,
        LogicalStyleValueState logicalState)
    {
        std::size_t matchingExamples = 0;

        for (const StyleLogicalStateExample& example : report.logicalStateExamples())
        {
            if (example.feature == feature && example.logicalState == logicalState)
            {
                ++matchingExamples;
            }
        }

        return report.logicalStateExamples().size() < kMaxRecordedExamples &&
            matchingExamples < 1;
    }

    std::string buildColorExampleDetail(
        const char* label,
        const std::optional<Color>& authoredColor,
        const std::optional<Color>& presentedColor)
    {
        std::ostringstream detail;
        detail
            << label
            << " authored=" << colorToString(authoredColor)
            << ", presented=" << colorToString(presentedColor);
        return detail.str();
    }

    std::string buildTextExampleDetail(
        const char* label,
        LogicalStyleValueState logicalState,
        bool presentedEnabled)
    {
        std::ostringstream detail;
        detail
            << label
            << " logicalState=" << CapabilityReport::toString(logicalState)
            << ", presented=" << (presentedEnabled ? "true" : "false");
        return detail.str();
    }
}

TerminalRenderer::TerminalRenderer() = default;

TerminalRenderer::~TerminalRenderer()
{
    shutdown();
}

bool TerminalRenderer::initialize()
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

    if (!configureTerminal())
    {
        restoreTerminalState();
        return false;
    }

    if (!queryVisibleConsoleSize(m_consoleWidth, m_consoleHeight))
    {
        m_consoleWidth = 120;
        m_consoleHeight = 40;
    }

    m_previousFrame.resize(m_consoleWidth, m_consoleHeight);
    m_previousFrame.clear();

    m_stylePolicy = buildStylePolicyFromCapabilities(m_capabilities);
    initializeDiagnosticsState();

    m_vtStyleState.reset();
    m_firstPresent = true;
    m_initialized = true;

    clearScreen();
    writeRaw("\x1b[?25l");

    flushDiagnosticsReport();

    return true;
}

void TerminalRenderer::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    flushDiagnosticsReport();
    resetStyle();
    writeRaw("\x1b[?25h");
    restoreTerminalState();

    m_initialized = false;
    m_firstPresent = true;
    m_vtStyleState.reset();
}

void TerminalRenderer::present(const ScreenBuffer& frame)
{
    if (!m_initialized)
    {
        throw std::runtime_error("TerminalRenderer::present called before initialize().");
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

void TerminalRenderer::resize(int width, int height)
{
    m_consoleWidth = width;
    m_consoleHeight = height;

    m_previousFrame.resize(width, height);
    m_previousFrame.clear();

    m_firstPresent = true;
    m_vtStyleState.reset();
}

bool TerminalRenderer::pollResize()
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

int TerminalRenderer::getConsoleWidth() const
{
    return m_consoleWidth;
}

int TerminalRenderer::getConsoleHeight() const
{
    return m_consoleHeight;
}

TextBackendCapabilities TerminalRenderer::textCapabilities() const
{
    TextBackendCapabilities capabilities;
    capabilities.supportsUtf16Output = false;
    capabilities.supportsCombiningMarks = false;
    capabilities.supportsEastAsianWide = true;
    capabilities.supportsEmoji = false;
    capabilities.supportsFontFallback = false;
    return capabilities;
}

void TerminalRenderer::setDiagnosticsEnabled(bool enabled)
{
    m_renderDiagnostics.setEnabled(enabled);
}

bool TerminalRenderer::diagnosticsEnabled() const
{
    return m_renderDiagnostics.isEnabled();
}

void TerminalRenderer::setDiagnosticsOutputPath(const std::string& outputPath)
{
    m_renderDiagnostics.setOutputPath(outputPath);
}

const std::string& TerminalRenderer::diagnosticsOutputPath() const
{
    return m_renderDiagnostics.outputPath();
}

void TerminalRenderer::setDiagnosticsAppendMode(bool appendMode)
{
    m_renderDiagnostics.setAppendMode(appendMode);
}

void TerminalRenderer::setStartupDiagnosticsContext(const StartupDiagnosticsContext& startupDiagnostics)
{
    m_startupDiagnostics = startupDiagnostics;
}

bool TerminalRenderer::diagnosticsAppendMode() const
{
    return m_renderDiagnostics.appendMode();
}

RenderDiagnostics& TerminalRenderer::diagnostics()
{
    return m_renderDiagnostics;
}

const RenderDiagnostics& TerminalRenderer::diagnostics() const
{
    return m_renderDiagnostics;
}

void TerminalRenderer::writeFullFrame(const ScreenBuffer& frame)
{
    VtFrameEmitter emitter(m_vtStyleState);
    emitter.beginFrame(true);

    for (int y = 0; y < frame.getHeight(); ++y)
    {
        appendSpanRuns(emitter, frame, y, 0, frame.getWidth() - 1);
    }

    writeRaw(emitter.finishFrame(false));
}

void TerminalRenderer::writeDirtySpans(const ScreenBuffer& frame)
{
    const std::vector<DirtySpan> spans = FrameDiff::diffRows(m_previousFrame, frame);

    if (spans.empty())
    {
        return;
    }

    VtFrameEmitter emitter(m_vtStyleState);
    emitter.beginFrame(false);

    for (const DirtySpan& span : spans)
    {
        appendSpanRuns(emitter, frame, span.y, span.xStart, span.xEnd);
    }

    writeRaw(emitter.finishFrame(false));
}

void TerminalRenderer::appendSpanRuns(
    VtFrameEmitter& emitter,
    const ScreenBuffer& frame,
    int y,
    int xStart,
    int xEnd)
{
    if (y < 0 || y >= frame.getHeight())
    {
        return;
    }

    if (frame.getWidth() <= 0)
    {
        return;
    }

    frame.expandSpanToGlyphBoundaries(y, xStart, xEnd);

    if (xStart > xEnd)
    {
        return;
    }

    int nextX = xStart;

    while (nextX <= xEnd)
    {
        VtRun run = buildRun(frame, y, nextX, xEnd, nextX);
        if (!run.empty())
        {
            emitter.appendRun(run);
        }
    }
}

VtRun TerminalRenderer::buildRun(
    const ScreenBuffer& frame,
    int y,
    int xStart,
    int xEnd,
    int& nextX)
{
    while (xStart <= xEnd && isContinuationCell(frame.getCell(xStart, y)))
    {
        ++xStart;
    }

    nextX = xStart;

    if (xStart > xEnd)
    {
        return VtRun{};
    }

    const ScreenCell& firstCell = frame.getCell(xStart, y);
    const Style authoredStyle = firstCell.style;
    const ResolvedStyle resolved = m_stylePolicy.resolve(authoredStyle);

    recordStyleUsage(authoredStyle, resolved);

    VtRun run;
    run.y = y;
    run.xStart = xStart;
    run.authoredStyle = authoredStyle;
    run.presentedStyle = resolved.presentedStyle;

    std::u32string runText;

    int x = xStart;
    while (x <= xEnd)
    {
        const ScreenCell& cell = frame.getCell(x, y);

        if (isContinuationCell(cell))
        {
            ++x;
            continue;
        }

        if (cell.style != authoredStyle)
        {
            break;
        }

        const char32_t glyph = cellToPresentedGlyph(cell);
        if (glyph != U'\0')
        {
            runText.push_back(glyph);
            run.cellWidth += glyphCellAdvance(cell);
        }

        x += glyphCellAdvance(cell);
        if (x <= xStart)
        {
            ++x;
        }
    }

    run.utf8Text = UnicodeConversion::u32ToUtf8(runText);
    nextX = x;

    return run;
}

void TerminalRenderer::resetStyle()
{
    std::string out;
    m_vtStyleState.appendReset(out);
    writeRaw(out);
}

bool TerminalRenderer::queryVisibleConsoleSize(int& width, int& height) const
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

bool TerminalRenderer::configureTerminal()
{
    if (!SetConsoleOutputCP(CP_UTF8))
    {
        return false;
    }

    if (!SetConsoleCP(CP_UTF8))
    {
        return false;
    }

    const TerminalCapabilityDetectionResult detection =
        TerminalCapabilityDetector::detectAndConfigure(
            m_hOut,
            m_hIn,
            m_originalOutputMode,
            m_originalInputMode,
            m_haveOriginalOutputMode,
            m_haveOriginalInputMode);

    m_capabilities = detection.capabilities;

    m_virtualTerminalEnabled = detection.virtualTerminalWasEnabled;
    m_virtualTerminalEnableAttempted = detection.virtualTerminalEnableAttempted;
    m_virtualTerminalEnableSucceeded = detection.virtualTerminalEnableSucceeded;

    m_configuredOutputMode = detection.configuredOutputMode;
    m_configuredInputMode = detection.configuredInputMode;
    m_haveConfiguredOutputMode = detection.hasConfiguredOutputMode;
    m_haveConfiguredInputMode = detection.hasConfiguredInputMode;

    return detection.terminalOutputReady;
}

void TerminalRenderer::restoreTerminalState()
{
    if (m_hOut != INVALID_HANDLE_VALUE && m_hOut != nullptr)
    {
        if (m_haveOriginalOutputMode)
        {
            SetConsoleMode(m_hOut, m_originalOutputMode);
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

void TerminalRenderer::initializeDiagnosticsState()
{
    m_renderDiagnostics.resetRuntimeData();

    BackendStateSnapshot backendState;
    backendState.startupConfigSource = m_startupDiagnostics.startupConfigSource;
    backendState.startupConfigFileFound = m_startupDiagnostics.startupConfigFileFound;
    backendState.configuredHostPreference = toString(m_startupDiagnostics.configuredHostPreference);
    backendState.configuredRendererPreference = toString(m_startupDiagnostics.configuredRendererPreference);

    backendState.requestedHostKind = toString(m_startupDiagnostics.requestedHost);
    backendState.actualHostKind = toString(m_startupDiagnostics.actualHost);
    backendState.requestedRendererIdentity = toString(m_startupDiagnostics.requestedRenderer);

    if (m_startupDiagnostics.actualRenderer != RendererKind::Unknown)
    {
        backendState.rendererIdentity = toString(m_startupDiagnostics.actualRenderer);
    }
    else
    {
        backendState.rendererIdentity = m_rendererIdentity;
    }

    backendState.activeRenderPath = "VirtualTerminalSequencePath";

    backendState.relaunchAttempted = m_startupDiagnostics.relaunchAttempted;
    backendState.relaunchPerformed = m_startupDiagnostics.relaunchPerformed;
    backendState.launchedByWindowsTerminalFlag = m_startupDiagnostics.launchedByWindowsTerminalFlag;
    backendState.windowsTerminalSessionHint = m_startupDiagnostics.windowsTerminalSessionHint;

    backendState.virtualTerminalEnableAttempted = m_virtualTerminalEnableAttempted;
    backendState.virtualTerminalEnableSucceeded = m_virtualTerminalEnableSucceeded;
    backendState.virtualTerminalProcessingActive = m_virtualTerminalEnableSucceeded;
    backendState.activeRenderPathUsesVirtualTerminalOutput = true;

    backendState.configuredOutputMode = 0;
    backendState.configuredInputMode = 0;
    backendState.hasConfiguredOutputMode = false;
    backendState.hasConfiguredInputMode = false;

    CapabilityReport& report = m_renderDiagnostics.report();
    report.setCapabilities(m_capabilities);
    report.setPolicy(m_stylePolicy);
    report.setBackendState(backendState);
}

void TerminalRenderer::flushDiagnosticsReport() const
{
    if (!m_renderDiagnostics.isEnabled())
    {
        return;
    }

    const bool wroteSuccessfully = RenderDiagnosticsWriter::write(m_renderDiagnostics);
    (void)wroteSuccessfully;
}

void TerminalRenderer::writeRaw(std::string_view text)
{
    if (text.empty())
    {
        return;
    }

    DWORD written = 0;
    WriteFile(
        m_hOut,
        text.data(),
        static_cast<DWORD>(text.size()),
        &written,
        nullptr);
}

void TerminalRenderer::clearScreen()
{
    writeRaw("\x1b[2J\x1b[H");
    m_vtStyleState.reset();
}

void TerminalRenderer::recordStyleUsage(const Style& authoredStyle, const ResolvedStyle& resolvedStyle)
{
    if (!m_renderDiagnostics.isEnabled())
    {
        return;
    }

    const Style& presentedStyle = resolvedStyle.presentedStyle;

    recordColorFeature(
        StyleFeature::ForegroundColor,
        authoredStyle.foreground(),
        presentedStyle.foreground());

    recordColorFeature(
        StyleFeature::BackgroundColor,
        authoredStyle.background(),
        presentedStyle.background());

    recordTextFeature(
        StyleFeature::Bold,
        authoredStyle.boldState(),
        presentedStyle.bold(),
        presentedStyle.bold(),
        m_stylePolicy.boldMode());

    recordTextFeature(
        StyleFeature::Dim,
        authoredStyle.dimState(),
        presentedStyle.dim(),
        presentedStyle.dim(),
        m_stylePolicy.dimMode());

    recordTextFeature(
        StyleFeature::Underline,
        authoredStyle.underlineState(),
        presentedStyle.underline(),
        presentedStyle.underline(),
        m_stylePolicy.underlineMode());

    recordBlinkFeature(
        StyleFeature::SlowBlink,
        authoredStyle.slowBlinkState(),
        presentedStyle.slowBlink(),
        resolvedStyle.emulateSlowBlink,
        presentedStyle.slowBlink());

    recordBlinkFeature(
        StyleFeature::FastBlink,
        authoredStyle.fastBlinkState(),
        presentedStyle.fastBlink(),
        resolvedStyle.emulateFastBlink,
        presentedStyle.fastBlink());

    recordTextFeature(
        StyleFeature::Reverse,
        authoredStyle.reverseState(),
        presentedStyle.reverse(),
        presentedStyle.reverse(),
        m_stylePolicy.reverseMode());

    recordTextFeature(
        StyleFeature::Invisible,
        authoredStyle.invisibleState(),
        presentedStyle.invisible(),
        presentedStyle.invisible(),
        m_stylePolicy.invisibleMode());

    recordTextFeature(
        StyleFeature::Strike,
        authoredStyle.strikeState(),
        presentedStyle.strike(),
        presentedStyle.strike(),
        m_stylePolicy.strikeMode());
}

void TerminalRenderer::recordColorFeature(
    StyleFeature feature,
    const std::optional<Color>& authoredColor,
    const std::optional<Color>& presentedColor)
{
    if (!authoredColor.has_value())
    {
        return;
    }

    CapabilityReport& report = m_renderDiagnostics.report();

    if (!presentedColor.has_value())
    {
        report.recordOmitted(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Omitted))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Omitted,
                buildColorExampleDetail("Color omitted", authoredColor, presentedColor));
        }

        return;
    }

    if (*presentedColor == *authoredColor)
    {
        if (!rendererPhysicallyRendersColor(presentedColor))
        {
            report.recordLogicalOnly(feature);

            if (shouldCaptureExample(report, feature, StyleAdaptationKind::LogicalOnly))
            {
                report.addExample(
                    feature,
                    StyleAdaptationKind::LogicalOnly,
                    buildColorExampleDetail("Color preserved logically but not emitted by the terminal output path", authoredColor, presentedColor));
            }
        }
        else
        {
            report.recordDirect(feature);
        }

        return;
    }

    const bool tierChanged = authoredColor->kind() != presentedColor->kind();

    if (tierChanged)
    {
        report.recordDowngraded(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Downgraded))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Downgraded,
                buildColorExampleDetail("Color downgraded", authoredColor, presentedColor));
        }
    }
    else
    {
        report.recordApproximated(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Approximated))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Approximated,
                buildColorExampleDetail("Color approximated", authoredColor, presentedColor));
        }
    }
}

void TerminalRenderer::recordTextFeature(
    StyleFeature feature,
    const Style::AttributeState& authoredState,
    bool presentedEnabled,
    bool physicallyRendered,
    TextAttributeRenderMode renderMode)
{
    CapabilityReport& report = m_renderDiagnostics.report();
    const LogicalStyleValueState logicalState = logicalStateFromAttributeState(authoredState);

    if (shouldCaptureLogicalStateExample(report, feature, logicalState))
    {
        std::ostringstream detail;
        detail
            << "Logical field observed with state="
            << CapabilityReport::toString(logicalState)
            << ", presented=" << (presentedEnabled ? "true" : "false");
        report.addLogicalStateExample(feature, logicalState, detail.str());
    }

    if (!authoredState.has_value())
    {
        return;
    }

    if (!*authoredState)
    {
        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Direct))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Direct,
                buildTextExampleDetail("Explicit disable preserved", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    if (!presentedEnabled)
    {
        report.recordOmitted(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Omitted))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Omitted,
                buildTextExampleDetail("Style omitted", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    if (!physicallyRendered)
    {
        report.recordLogicalOnly(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::LogicalOnly))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::LogicalOnly,
                buildTextExampleDetail("Style preserved logically but not emitted by the terminal output path", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    if (renderMode == TextAttributeRenderMode::Approximate)
    {
        report.recordApproximated(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Approximated))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Approximated,
                buildTextExampleDetail("Style approximated through the current terminal output path", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    report.recordDirect(feature);

    if (shouldCaptureExample(report, feature, StyleAdaptationKind::Direct))
    {
        report.addExample(
            feature,
            StyleAdaptationKind::Direct,
            buildTextExampleDetail("Style rendered directly", logicalState, presentedEnabled),
            logicalState);
    }
}

void TerminalRenderer::recordBlinkFeature(
    StyleFeature feature,
    const Style::AttributeState& authoredState,
    bool presentedEnabled,
    bool emulated,
    bool physicallyRendered)
{
    CapabilityReport& report = m_renderDiagnostics.report();
    const LogicalStyleValueState logicalState = logicalStateFromAttributeState(authoredState);

    if (shouldCaptureLogicalStateExample(report, feature, logicalState))
    {
        std::ostringstream detail;
        detail
            << "Logical field observed with state="
            << CapabilityReport::toString(logicalState)
            << ", presented=" << (presentedEnabled ? "true" : "false");
        report.addLogicalStateExample(feature, logicalState, detail.str());
    }

    if (!authoredState.has_value())
    {
        return;
    }

    if (!*authoredState)
    {
        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Direct))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Direct,
                buildTextExampleDetail("Explicit blink disable preserved", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    if (emulated)
    {
        report.recordEmulated(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Emulated))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Emulated,
                buildTextExampleDetail("Emulated blink", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    if (!presentedEnabled)
    {
        report.recordOmitted(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::Omitted))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::Omitted,
                buildTextExampleDetail("Blink omitted", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    if (!physicallyRendered)
    {
        report.recordLogicalOnly(feature);

        if (shouldCaptureExample(report, feature, StyleAdaptationKind::LogicalOnly))
        {
            report.addExample(
                feature,
                StyleAdaptationKind::LogicalOnly,
                buildTextExampleDetail("Blink preserved logically but not emitted by the terminal output path", logicalState, presentedEnabled),
                logicalState);
        }

        return;
    }

    report.recordDirect(feature);

    if (shouldCaptureExample(report, feature, StyleAdaptationKind::Direct))
    {
        report.addExample(
            feature,
            StyleAdaptationKind::Direct,
            buildTextExampleDetail("Blink rendered directly", logicalState, presentedEnabled),
            logicalState);
    }
}
#pragma once

#include <optional>
#include <string>

#include "Rendering/Backends/ConsoleCapabilityDetector.h"
#include "Rendering/Capabilities/ConsoleCapabilities.h"
#include "Rendering/Diagnostics/RenderDiagnostics.h"
#include "Rendering/FrameDiff.h"
#include "Rendering/IRenderer.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Color.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/StylePolicy.h"
#include "Rendering/Text/TextTypes.h"

#define NOMINMAX
#include <windows.h>

/*
    Purpose:

    ConsoleRenderer is the Windows console backend/output layer.

    For Phase 1 Unicode readiness:
        - ScreenBuffer owns logical Unicode placement
        - Unicode conversion is centralized outside the renderer
        - the renderer writes visible runs only
        - continuation cells are skipped during presentation
        - backend capability reporting is exposed through IRenderer

    For Phase 2 style adaptation:
        - logical Style stays unchanged in ScreenBuffer
        - capability detection populates ConsoleCapabilities
        - StylePolicy resolves unsupported features at presentation time
        - backend mapping uses resolved presentation style only

    For Phase 2 structured diagnostics:
        - diagnostics are optional and renderer-owned
        - runtime adaptation decisions are recorded at the real style resolution point
        - author-facing hints are built from collected report data, not renderer internals
*/

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
    bool pollResize() override;

    int getConsoleWidth() const override;
    int getConsoleHeight() const override;

    TextBackendCapabilities textCapabilities() const override;

    void setDiagnosticsEnabled(bool enabled);
    bool diagnosticsEnabled() const;

    void setDiagnosticsOutputPath(const std::string& outputPath);
    const std::string& diagnosticsOutputPath() const;

    void setDiagnosticsAppendMode(bool appendMode);
    bool diagnosticsAppendMode() const;

    RenderDiagnostics& diagnostics();
    const RenderDiagnostics& diagnostics() const;

private:
    void writeFullFrame(const ScreenBuffer& frame);
    void writeDirtySpans(const ScreenBuffer& frame);
    void writeSpan(const ScreenBuffer& frame, int y, int xStart, int xEnd);

    void moveCursor(int x, int y);
    void setStyle(const Style& style);
    void resetStyle();

    bool queryVisibleConsoleSize(int& width, int& height) const;
    bool configureConsole();
    void restoreConsoleState();

    void initializeDiagnosticsState();
    void flushDiagnosticsReport() const;

    void recordStyleUsage(const Style& authoredStyle, const ResolvedStyle& resolvedStyle);
    void recordColorFeature(
        StyleFeature feature,
        const std::optional<Color>& authoredColor,
        const std::optional<Color>& presentedColor);
    void recordTextFeature(
        StyleFeature feature,
        bool authoredEnabled,
        bool presentedEnabled,
        bool physicallyRendered);
    void recordBlinkFeature(
        StyleFeature feature,
        bool authoredEnabled,
        bool presentedEnabled,
        bool emulated,
        bool physicallyRendered);

private:
    HANDLE m_hOut = INVALID_HANDLE_VALUE;
    HANDLE m_hIn = INVALID_HANDLE_VALUE;

    int m_consoleWidth = 0;
    int m_consoleHeight = 0;

    ScreenBuffer m_previousFrame;
    bool m_initialized = false;
    bool m_firstPresent = true;

    Style m_currentStyle{};
    StylePolicy m_stylePolicy{};
    ConsoleCapabilities m_capabilities{};
    RenderDiagnostics m_renderDiagnostics{};
    WORD m_defaultAttributes = 0;

    UINT m_originalOutputCodePage = 0;
    UINT m_originalInputCodePage = 0;

    DWORD m_originalOutputMode = 0;
    DWORD m_originalInputMode = 0;

    bool m_haveOriginalOutputMode = false;
    bool m_haveOriginalInputMode = false;
    bool m_cursorWasVisible = true;
    bool m_virtualTerminalEnabled = false;
};
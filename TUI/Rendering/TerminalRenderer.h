#pragma once

#include <string>
#include <string_view>

#include "Rendering/Backends/TerminalCapabilityDetector.h"
#include "Rendering/Capabilities/ConsoleCapabilities.h"
#include "Rendering/Diagnostics/RenderDiagnostics.h"
#include "Rendering/Diagnostics/StartupDiagnosticsContext.h"
#include "Rendering/FrameDiff.h"
#include "Rendering/IRenderer.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Color.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/StylePolicy.h"
#include "Rendering/Text/TextTypes.h"
#include "Rendering/VtFrameEmitter.h"
#include "Rendering/VtRun.h"
#include "Rendering/VtStyleState.h"

#define NOMINMAX
#include <windows.h>

class TerminalRenderer : public IRenderer
{
public:
    TerminalRenderer();
    ~TerminalRenderer() override;

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

    void setStartupDiagnosticsContext(const StartupDiagnosticsContext& startupDiagnostics);

    RenderDiagnostics& diagnostics();
    const RenderDiagnostics& diagnostics() const;

private:
    void writeFullFrame(const ScreenBuffer& frame);
    void writeDirtySpans(const ScreenBuffer& frame);

    void appendSpanRuns(
        VtFrameEmitter& emitter,
        const ScreenBuffer& frame,
        int y,
        int xStart,
        int xEnd);

    VtRun buildRun(
        const ScreenBuffer& frame,
        int y,
        int xStart,
        int xEnd,
        int& nextX);

    void resetStyle();

    bool queryVisibleConsoleSize(int& width, int& height) const;
    bool configureTerminal();
    void restoreTerminalState();

    void initializeDiagnosticsState();
    void flushDiagnosticsReport() const;

    void writeRaw(std::string_view text);
    void clearScreen();

    void recordStyleUsage(const Style& authoredStyle, const ResolvedStyle& resolvedStyle);
    void recordColorFeature(
        StyleFeature feature,
        const std::optional<Color>& authoredColor,
        const std::optional<Color>& presentedColor);
    void recordTextFeature(
        StyleFeature feature,
        const Style::AttributeState& authoredState,
        bool presentedEnabled,
        bool physicallyRendered,
        TextAttributeRenderMode renderMode);
    void recordBlinkFeature(
        StyleFeature feature,
        const Style::AttributeState& authoredState,
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

    VtStyleState m_vtStyleState{};
    StylePolicy m_stylePolicy{};
    ConsoleCapabilities m_capabilities{};
    RenderDiagnostics m_renderDiagnostics{};
    StartupDiagnosticsContext m_startupDiagnostics{};

    UINT m_originalOutputCodePage = 0;
    UINT m_originalInputCodePage = 0;

    DWORD m_originalOutputMode = 0;
    DWORD m_originalInputMode = 0;

    bool m_haveOriginalOutputMode = false;
    bool m_haveOriginalInputMode = false;
    bool m_cursorWasVisible = true;

    std::string m_rendererIdentity = "TerminalRenderer";
    bool m_virtualTerminalEnabled = false;
    bool m_virtualTerminalEnableAttempted = false;
    bool m_virtualTerminalEnableSucceeded = false;

    DWORD m_configuredOutputMode = 0;
    DWORD m_configuredInputMode = 0;
    bool m_haveConfiguredOutputMode = false;
    bool m_haveConfiguredInputMode = false;
};
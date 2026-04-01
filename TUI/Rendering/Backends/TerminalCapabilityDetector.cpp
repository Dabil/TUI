#include "Rendering/Backends/TerminalCapabilityDetector.h"

#include "Rendering/Backends/RendererCapabilityDetector.h"

TerminalCapabilityDetectionResult TerminalCapabilityDetector::detectAndConfigure(
    HANDLE hOut,
    HANDLE hIn,
    DWORD originalOutputMode,
    DWORD originalInputMode,
    bool haveOriginalOutputMode,
    bool haveOriginalInputMode)
{
    TerminalCapabilityDetectionResult result;

    const RendererCapabilityProbeResult probe =
        RendererCapabilityDetector::detectAndConfigure(
            hOut,
            hIn,
            originalOutputMode,
            originalInputMode,
            haveOriginalOutputMode,
            haveOriginalInputMode,
            true,
            true);

    result.virtualTerminalWasEnabled = probe.virtualTerminalWasEnabled;
    result.virtualTerminalEnableAttempted = probe.virtualTerminalEnableAttempted;
    result.virtualTerminalEnableSucceeded = probe.virtualTerminalEnableSucceeded;
    result.configuredOutputMode = probe.configuredOutputMode;
    result.configuredInputMode = probe.configuredInputMode;
    result.hasConfiguredOutputMode = probe.hasConfiguredOutputMode;
    result.hasConfiguredInputMode = probe.hasConfiguredInputMode;

    if (!probe.virtualTerminalWasEnabled)
    {
        return result;
    }

    result.capabilities = RendererCapabilities::VirtualTerminal();
    result.terminalOutputReady = true;
    return result;
}
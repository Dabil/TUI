#include "Rendering/Backends/TerminalCapabilityDetector.h"

TerminalCapabilityDetectionResult TerminalCapabilityDetector::detectAndConfigure(
    HANDLE hOut,
    HANDLE hIn,
    DWORD originalOutputMode,
    DWORD originalInputMode,
    bool haveOriginalOutputMode,
    bool haveOriginalInputMode)
{
    TerminalCapabilityDetectionResult result;

    if (!haveOriginalOutputMode)
    {
        return result;
    }

    DWORD outMode = originalOutputMode;
    outMode |= ENABLE_PROCESSED_OUTPUT;
    outMode |= ENABLE_WRAP_AT_EOL_OUTPUT;
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    result.virtualTerminalEnableAttempted = true;

    if (!SetConsoleMode(hOut, outMode))
    {
        return result;
    }

    result.virtualTerminalEnableSucceeded = true;
    result.virtualTerminalWasEnabled = true;
    result.configuredOutputMode = outMode;
    result.hasConfiguredOutputMode = true;

    if (haveOriginalInputMode)
    {
        DWORD inMode = originalInputMode;
        inMode |= ENABLE_PROCESSED_INPUT;
        inMode |= ENABLE_WINDOW_INPUT;
        inMode |= ENABLE_EXTENDED_FLAGS;
        inMode &= ~ENABLE_QUICK_EDIT_MODE;
        inMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

        if (!SetConsoleMode(hIn, inMode))
        {
            return result;
        }

        result.configuredInputMode = inMode;
        result.hasConfiguredInputMode = true;
    }

    result.capabilities = RendererCapabilities::VirtualTerminal();
    result.terminalOutputReady = true;
    return result;
}
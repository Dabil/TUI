#include "Rendering/Backends/ConsoleCapabilityDetector.h"

namespace
{
    RendererCapabilities buildConservativeCapabilities(bool virtualTerminalEnabled)
    {
        /*
            Important architectural choice:

            The current ConsoleRenderer still renders through Win32 console
            attributes via SetConsoleTextAttribute, not through VT escape output.

            Therefore, even if VT processing can be enabled, the active output
            path should still be described conservatively as BasicWin32 until a
            true VT presentation path is added later.

            In particular, VT enablement alone must not be treated as evidence
            that the current renderer can defensibly promise richer direct style
            semantics. The capability object should describe the active render
            path, not the broadest mode bit that happened to be enabled.
        */
        RendererCapabilities capabilities = RendererCapabilities::BasicWin32();
        capabilities.virtualTerminalProcessing = virtualTerminalEnabled;
        return capabilities;
    }
}

ConsoleCapabilityDetectionResult ConsoleCapabilityDetector::detectAndConfigure(
    HANDLE hOut,
    HANDLE hIn,
    DWORD originalOutputMode,
    DWORD originalInputMode,
    bool haveOriginalOutputMode,
    bool haveOriginalInputMode)
{
    ConsoleCapabilityDetectionResult result;

    bool virtualTerminalEnabled = false;

    if (haveOriginalOutputMode)
    {
        DWORD outMode = originalOutputMode;
        outMode |= ENABLE_PROCESSED_OUTPUT;
        outMode |= ENABLE_WRAP_AT_EOL_OUTPUT;
        outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

        result.virtualTerminalEnableAttempted = true;

        if (SetConsoleMode(hOut, outMode))
        {
            virtualTerminalEnabled = true;
            result.virtualTerminalEnableSucceeded = true;
            result.virtualTerminalWasEnabled = true;
            result.configuredOutputMode = outMode;
            result.hasConfiguredOutputMode = true;
        }
        else
        {
            DWORD fallbackMode = originalOutputMode;
            fallbackMode |= ENABLE_PROCESSED_OUTPUT;
            fallbackMode |= ENABLE_WRAP_AT_EOL_OUTPUT;

            if (!SetConsoleMode(hOut, fallbackMode))
            {
                return result;
            }

            result.configuredOutputMode = fallbackMode;
            result.hasConfiguredOutputMode = true;
        }
    }

    if (haveOriginalInputMode)
    {
        DWORD inMode = originalInputMode;
        inMode |= ENABLE_PROCESSED_INPUT;
        inMode |= ENABLE_WINDOW_INPUT;
        inMode |= ENABLE_EXTENDED_FLAGS;
        inMode &= ~ENABLE_QUICK_EDIT_MODE;

        if (!SetConsoleMode(hIn, inMode))
        {
            return result;
        }

        result.configuredInputMode = inMode;
        result.hasConfiguredInputMode = true;
    }

    result.capabilities = buildConservativeCapabilities(virtualTerminalEnabled);
    return result;
}

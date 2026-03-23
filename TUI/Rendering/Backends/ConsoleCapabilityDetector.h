#pragma once

#include "Rendering/Capabilities/ConsoleCapabilities.h"

#define NOMINMAX
#include <windows.h>

/*
    Purpose:

    ConsoleCapabilityDetector is the Windows-backend-specific capability probe.

    It is responsible for:
        - querying current console modes
        - attempting safe VT enablement
        - reporting conservative backend capabilities
        - keeping platform detection out of logical style/theme code

    Important:
        This detector reports what the active backend path can safely support.
        Since the current renderer still uses Win32 attribute presentation,
        capability reporting remains conservative even if VT mode can be enabled.
*/

struct ConsoleCapabilityDetectionResult
{
    ConsoleCapabilities capabilities;

    bool virtualTerminalWasEnabled = false;
    bool virtualTerminalEnableAttempted = false;
    bool virtualTerminalEnableSucceeded = false;

    DWORD configuredOutputMode = 0;
    DWORD configuredInputMode = 0;

    bool hasConfiguredOutputMode = false;
    bool hasConfiguredInputMode = false;
};

class ConsoleCapabilityDetector
{
public:
    static ConsoleCapabilityDetectionResult detectAndConfigure(
        HANDLE hOut,
        HANDLE hIn,
        DWORD originalOutputMode,
        DWORD originalInputMode,
        bool haveOriginalOutputMode,
        bool haveOriginalInputMode);
};
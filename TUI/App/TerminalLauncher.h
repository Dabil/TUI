#pragma once

#include "App/StartupConfig.h"
#include "Rendering/Diagnostics/StartupDiagnosticsContext.h"

enum class StartupRendererSelection
{
    Console = 0,
    Terminal
};

struct StartupLaunchDecision
{
    bool relaunchPerformed = false;
    StartupRendererSelection rendererSelection = StartupRendererSelection::Console;
    StartupDiagnosticsContext diagnosticsContext{};
};

class TerminalLauncher
{
public:
    static StartupLaunchDecision prepareStartup(const StartupConfig& config);

private:
    static bool tryRelaunchInWindowsTerminal();
};
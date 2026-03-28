#pragma once

class TerminalLauncher
{
public:
    static bool tryRelaunchInWindowsTerminal();
    static bool shouldPreferTerminalRenderer();
};
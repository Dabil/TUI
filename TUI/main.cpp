#include "App/Application.h"
#include "App/TerminalLauncher.h"

/*
One very important Windows console rule

The order matters:

1) Set buffer size
2) Set window size

If the window is larger than the buffer, Windows will fail.
*/

int main()
{
    if (TerminalLauncher::tryRelaunchInWindowsTerminal())
    {
        return 0;
    }

    Application app;

    if (!app.initialize())
    {
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}
#include "App/Application.h"
#include "App/StartupConfig.h"
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
    const StartupConfig startupConfig = StartupConfigLoader::loadFromStartupIni();
    const StartupLaunchDecision launchDecision = TerminalLauncher::prepareStartup(startupConfig);

    if (launchDecision.relaunchPerformed)
    {
        return 0;
    }

    Application app(
        launchDecision.rendererSelection,
        launchDecision.diagnosticsContext);

    if (!app.initialize())
    {
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}
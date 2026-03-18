#include "App/Application.h"

/*
One very important Windows console rule

The order matters:

1) Set buffer size
2) Set window size

If the window is larger than the buffer, Windows will fail.
*/

int main()
{
    Application app;

    if (!app.initialize())
    {
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}
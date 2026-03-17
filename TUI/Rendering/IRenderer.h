#pragma once

class ScreenBuffer;

/*
    IRenderer exists to separate the rendering engine from the specific
    output system (console, SDLwindow, web terminal, etc)

    IRenderer creates an abstraction layer between the TUI engine and 
    the Rendering implementation. So this makes adding additional
    backends clean and neat. 

    With IRenderer the engine depends on the interface:
    std::unique_ptr<IRenderer> renderer;

    Then at runtime you decide which renderer to use:
        renderer = std::make_unique<ConsoleRenderer>();
        renderer = std::make_unique<SDLRenderer();
        renderer = std::make_unique<WebRenderer();
        etc...

        The engine does not know or care what type of renderer is
        behind it. The interface describes what a renderer must
        be able to do. So any renderer must implement IRenderer 
        functions.

        Because the interface has been standardized, the engine code
        doesn't change at all.The engine becomes platform independent. 
*/

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void present(const ScreenBuffer& frame) = 0;
    virtual void resize(int width, int height) = 0;
    virtual bool pollResize() = 0;

    virtual int getConsoleWidth() const = 0;
    virtual int getConsoleHeight() const = 0;
};
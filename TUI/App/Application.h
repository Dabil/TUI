#pragma once

#include <memory>

/*
    Update:

    Change only if you want renderer-agnostic naming later. 
    For Phase 1 Unicode, almost no change

    Checklist:
        - No Unicode-specific change required
*/

class ScreenManager;
class IRenderer;
class Surface;

class Application
{
public:
    Application();
    ~Application();

    bool initialize();
    void run();
    void shutdown();

private:
    void handleResize();
    void update(double deltaTime);
    void render();

private:
    std::unique_ptr<ScreenManager> m_screenManager;
    std::unique_ptr<IRenderer> m_renderer;
    std::unique_ptr<Surface> m_surface;

    bool m_running = false;

    int m_width = 0;
    int m_height = 0;
};
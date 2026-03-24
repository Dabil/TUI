#pragma once

#include <memory>

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
    enum class ScreenType
    {
        WaterEffect,
        Donut3D,
        Starfield
    };

private:
    void handleResize();
    void update(double deltaTime);
    void render();

    void switchToScreen(ScreenType screenType);
    void advanceScreen();
    void updateScreenCycle(double deltaTime);

private:
    std::unique_ptr<ScreenManager> m_screenManager;
    std::unique_ptr<IRenderer> m_renderer;
    std::unique_ptr<Surface> m_surface;

    bool m_running = false;

    int m_width = 0;
    int m_height = 0;

    ScreenType m_currentScreenType = ScreenType::WaterEffect;
    double m_screenCycleElapsedSeconds = 0.0;
    double m_screenCycleIntervalSeconds = 60.0;
};
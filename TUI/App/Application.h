#pragma once

#include <memory>

#include "App/TerminalLauncher.h"
#include "Assets/AssetLibrary.h"
#include "Rendering/Diagnostics/StartupDiagnosticsContext.h"

class ScreenManager;
class IRenderer;
class Surface;

namespace Input
{
    class InputManager;
}

class Application
{
public:
    explicit Application(
        StartupRendererSelection rendererSelection,
        StartupValidationScreenPreference validationScreenSelection,
        const StartupDiagnosticsContext& startupDiagnostics);
    ~Application();

    bool initialize();
    void run();
    void shutdown();

private:
    enum class ScreenType
    {
        TerminalCapabilities,
        RendererDiagnostics,
        ControlDeck,
        RetroTerminal,
        NeonDialog,
        OpsWall,
        DigitalRain,
        WaterEffect,
        Donut3D,
        Fire
    };

private:
    void handleResize();
    void update(double deltaTime);
    void render();

    void switchToScreen(ScreenType screenType);
    void advanceScreen();
    void updateScreenCycle(double deltaTime);

    void configureAssetLibrary();

private:
    StartupRendererSelection m_rendererSelection = StartupRendererSelection::Console;
    StartupValidationScreenPreference m_validationScreenStart = StartupValidationScreenPreference::ValidationStartFalse;
    StartupDiagnosticsContext m_startupDiagnostics{};

    std::unique_ptr<ScreenManager> m_screenManager;
    std::unique_ptr<IRenderer> m_renderer;
    std::unique_ptr<Surface> m_surface;
    std::unique_ptr<Input::InputManager> m_inputManager;

    Assets::AssetLibrary m_assetLibrary;

    bool m_running = false;

    int m_width = 0;
    int m_height = 0;

    // default start-up screen, but the actual is declared in Application.cpp/initialize
    ScreenType m_currentScreenType = ScreenType::ControlDeck;
    double m_screenCycleElapsedSeconds = 0.0;
    double m_screenCycleIntervalSeconds = 20.0;
};
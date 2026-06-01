#pragma once

#include "App/ApplicationHost.h"
#include "Animation/TickEvent.h"
#include "Input/Event.h"

class DemoApplication : public ApplicationHost
{
public:
    explicit DemoApplication(
        StartupRendererSelection rendererSelection,
        StartupValidationScreenPreference validationScreenSelection,
        const StartupDiagnosticsContext& startupDiagnostics);

    ~DemoApplication() override = default;

protected:
    void configureAssetLibrary() override;
    bool onInitialize() override;

    bool handleApplicationCommand(const Input::CommandEvent& commandEvent) override;
    void update(const Animation::TickEvent& event) override;

private:
    enum class ScreenType
    {
        TerminalCapabilities,
        RendererDiagnostics,
        ControlDeck,
        RetroTerminal,
        NeonDialog,
        OpsWall,
        MenuDemoScreen,
        WidgetDemoScreen,
        ScrollableTileGridDemo,
        WindowDemo,
        DigitalRain,
        WaterEffect,
        Donut3D,
        Fire
    };

private:
    void switchToScreen(ScreenType screenType);
    void advanceScreen();
    void previousScreen();
    void updateScreenCycle(const Animation::TickEvent& event);

private:
    StartupValidationScreenPreference m_validationScreenStart =
        StartupValidationScreenPreference::ValidationStartFalse;

    ScreenType m_currentScreenType = ScreenType::ControlDeck;

    double m_screenCycleElapsedSeconds = 0.0;
    double m_screenCycleIntervalSeconds = 60.0;
};


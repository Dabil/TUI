#pragma once

#include <string>

#include "Animation/TickEvent.h"
#include "App/ApplicationHost.h"
#include "Input/Event.h"
#include "UI/Widgets/ContainerWidget.h"

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

    bool dispatchEvent(const Input::Event& event) override;
    bool handleApplicationCommand(const Input::CommandEvent& commandEvent) override;

    void update(const Animation::TickEvent& event) override;
    void render() override;

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
    void registerDemoRoutes();

    void switchToScreen(ScreenType screenType);
    void advanceScreen();
    void previousScreen();

    ScreenType nextScreenType(ScreenType screenType) const;
    ScreenType previousScreenType(ScreenType screenType) const;

    std::string routeIdForScreenType(ScreenType screenType) const;

    void rebuildNavigationButtons();
    void positionNavigationButtons();
    bool dispatchNavigationCommandId(const std::string& commandId);

    void updateScreenCycle(const Animation::TickEvent& event);

private:
    StartupValidationScreenPreference m_validationScreenStart =
        StartupValidationScreenPreference::ValidationStartFalse;

    ScreenType m_currentScreenType = ScreenType::ControlDeck;

    double m_screenCycleElapsedSeconds = 0.0;
    double m_screenCycleIntervalSeconds = 60.0;

    ContainerWidget m_navigationButtons;
};
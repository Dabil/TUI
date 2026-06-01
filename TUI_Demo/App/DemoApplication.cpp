#include "App/DemoApplication.h"

#include "App/ScreenManager.h"

#include "Screens/Developer/RendererDiagnosticsScreen.h"
#include "Screens/Developer/TerminalCapabilitiesScreen.h"
#include "Screens/DigitalRainScreen.h"
#include "Screens/Donut3DScreen.h"
#include "Screens/FireScreen.h"
#include "Screens/MenuDemoScreen.h"
#include "Screens/ScrollableTileGridDemoScreen.h"
#include "Screens/ShowcaseScreen.h"
#include "Screens/WaterEffectScreen.h"
#include "Screens/WidgetDemoScreen.h"
#include "Screens/WindowDemoScreen.h"

#include <memory>

DemoApplication::DemoApplication(
    StartupRendererSelection rendererSelection,
    StartupValidationScreenPreference validationScreenSelection,
    const StartupDiagnosticsContext& startupDiagnostics)
    : ApplicationHost(rendererSelection, validationScreenSelection, startupDiagnostics)
    , m_validationScreenStart(validationScreenSelection)
{
}

void DemoApplication::configureAssetLibrary()
{
    assetLibrary().setAssetsRoot("Assets");

    assetLibrary().registerAsset(
        "banner.fire_font_k",
        "Fonts/FIGlet/Fire Font-k.flf");

    assetLibrary().registerAsset(
        "banner.ansi_shadow",
        "Fonts/FIGlet/ANSI Shadow.flf");

    assetLibrary().registerAsset(
        "banner.computer",
        "Fonts/Figlet/Computer.flf");

    assetLibrary().registerAsset(
        "pfont.assemble_box",
        "Fonts/pFont/AssembleBox.pfont");

    assetLibrary().registerAsset(
        "xp.retro_terminal_1",
        "xp/retroTerm1.xp");
}

bool DemoApplication::onInitialize()
{
    if (m_validationScreenStart == StartupValidationScreenPreference::ValidationStartTrue)
    {
        m_currentScreenType = ScreenType::TerminalCapabilities;
    }
    else
    {
        m_currentScreenType = ScreenType::ControlDeck;
    }

    m_screenCycleElapsedSeconds = 0.0;
    switchToScreen(m_currentScreenType);

    return true;
}

bool DemoApplication::handleApplicationCommand(const Input::CommandEvent& commandEvent)
{
    switch (commandEvent.command.code)
    {
    case Input::CommandCode::Confirm:
    case Input::CommandCode::Forward:
    case Input::CommandCode::MoveRight:
        advanceScreen();
        return true;

    case Input::CommandCode::Back:
    case Input::CommandCode::MoveLeft:
        previousScreen();
        return true;

    default:
        return ApplicationHost::handleApplicationCommand(commandEvent);
    }
}

void DemoApplication::update(const Animation::TickEvent& event)
{
    updateScreenCycle(event);

    ApplicationHost::update(event);
}

void DemoApplication::switchToScreen(ScreenType screenType)
{
    screenManager().clearScreens();

    switch (screenType)
    {
    case ScreenType::TerminalCapabilities:
        screenManager().pushScreen(std::make_unique<TerminalCapabilitiesScreen>(&renderer()));
        break;

    case ScreenType::RendererDiagnostics:
        screenManager().pushScreen(std::make_unique<RendererDiagnosticsScreen>(&renderer()));
        break;

    case ScreenType::ControlDeck:
        screenManager().pushScreen(std::make_unique<ControlDeckScreen>());
        break;

    case ScreenType::RetroTerminal:
        screenManager().pushScreen(std::make_unique<RetroTerminalScreen>(assetLibrary()));
        break;

    case ScreenType::NeonDialog:
        screenManager().pushScreen(std::make_unique<NeonDialogScreen>());
        break;

    case ScreenType::OpsWall:
        screenManager().pushScreen(std::make_unique<OpsWallScreen>());
        break;

    case ScreenType::MenuDemoScreen:
        screenManager().pushScreen(std::make_unique<MenuDemoScreen>());
        break;

    case ScreenType::WidgetDemoScreen:
        screenManager().pushScreen(std::make_unique<WidgetDemoScreen>());
        break;

    case ScreenType::ScrollableTileGridDemo:
        screenManager().pushScreen(std::make_unique<ScrollableTileGridDemoScreen>());
        break;

    case ScreenType::WindowDemo:
        screenManager().pushScreen(std::make_unique<WindowDemo>());
        break;

    case ScreenType::DigitalRain:
        screenManager().pushScreen(
            std::make_unique<DigitalRainScreen>(startupDiagnostics().actualHost));
        break;

    case ScreenType::WaterEffect:
        screenManager().pushScreen(std::make_unique<WaterEffectScreen>(assetLibrary()));
        break;

    case ScreenType::Donut3D:
        screenManager().pushScreen(std::make_unique<Donut3DScreen>());
        break;

    case ScreenType::Fire:
        screenManager().pushScreen(std::make_unique<FireScreen>(assetLibrary()));
        break;
    }

    m_currentScreenType = screenType;
    m_screenCycleElapsedSeconds = 0.0;
    requestRender();
}

void DemoApplication::advanceScreen()
{
    switch (m_currentScreenType)
    {
    case ScreenType::ControlDeck:
        switchToScreen(ScreenType::RetroTerminal);
        break;

    case ScreenType::RetroTerminal:
        switchToScreen(ScreenType::NeonDialog);
        break;

    case ScreenType::NeonDialog:
        switchToScreen(ScreenType::OpsWall);
        break;

    case ScreenType::OpsWall:
        switchToScreen(ScreenType::MenuDemoScreen);
        break;

    case ScreenType::MenuDemoScreen:
        switchToScreen(ScreenType::WidgetDemoScreen);
        break;

    case ScreenType::WidgetDemoScreen:
        switchToScreen(ScreenType::ScrollableTileGridDemo);
        break;

    case ScreenType::ScrollableTileGridDemo:
        switchToScreen(ScreenType::WindowDemo);
        break;

    case ScreenType::WindowDemo:
        switchToScreen(ScreenType::DigitalRain);
        break;

    case ScreenType::DigitalRain:
        switchToScreen(ScreenType::WaterEffect);
        break;

    case ScreenType::WaterEffect:
        switchToScreen(ScreenType::Donut3D);
        break;

    case ScreenType::Donut3D:
        switchToScreen(ScreenType::Fire);
        break;

    case ScreenType::Fire:
        switchToScreen(ScreenType::ControlDeck);
        break;

    case ScreenType::TerminalCapabilities:
        switchToScreen(ScreenType::RendererDiagnostics);
        break;

    case ScreenType::RendererDiagnostics:
        switchToScreen(ScreenType::TerminalCapabilities);
        break;
    }
}

void DemoApplication::previousScreen()
{
    switch (m_currentScreenType)
    {
    case ScreenType::ControlDeck:
        switchToScreen(ScreenType::Fire);
        break;

    case ScreenType::RetroTerminal:
        switchToScreen(ScreenType::ControlDeck);
        break;

    case ScreenType::NeonDialog:
        switchToScreen(ScreenType::RetroTerminal);
        break;

    case ScreenType::OpsWall:
        switchToScreen(ScreenType::NeonDialog);
        break;

    case ScreenType::MenuDemoScreen:
        switchToScreen(ScreenType::OpsWall);
        break;

    case ScreenType::WidgetDemoScreen:
        switchToScreen(ScreenType::MenuDemoScreen);
        break;

    case ScreenType::ScrollableTileGridDemo:
        switchToScreen(ScreenType::WidgetDemoScreen);
        break;

    case ScreenType::WindowDemo:
        switchToScreen(ScreenType::ScrollableTileGridDemo);
        break;

    case ScreenType::DigitalRain:
        switchToScreen(ScreenType::WindowDemo);
        break;

    case ScreenType::WaterEffect:
        switchToScreen(ScreenType::DigitalRain);
        break;

    case ScreenType::Donut3D:
        switchToScreen(ScreenType::WaterEffect);
        break;

    case ScreenType::Fire:
        switchToScreen(ScreenType::Donut3D);
        break;

    case ScreenType::TerminalCapabilities:
        switchToScreen(ScreenType::RendererDiagnostics);
        break;

    case ScreenType::RendererDiagnostics:
        switchToScreen(ScreenType::TerminalCapabilities);
        break;
    }
}

void DemoApplication::updateScreenCycle(const Animation::TickEvent& event)
{
    m_screenCycleElapsedSeconds += event.deltaSeconds;

    while (m_screenCycleElapsedSeconds >= m_screenCycleIntervalSeconds)
    {
        m_screenCycleElapsedSeconds -= m_screenCycleIntervalSeconds;
        advanceScreen();
    }
}


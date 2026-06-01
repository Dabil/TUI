#include "App/DemoApplication.h"

#include "App/ScreenManager.h"

#include "Rendering/IRenderer.h"
#include "Rendering/Surface.h"

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
#include "Rendering/Styles/Themes.h"

#include "UI/Widgets/Button.h"

#include <memory>
#include <string>
#include <utility>

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
    registerDemoRoutes();

    if (m_validationScreenStart == StartupValidationScreenPreference::ValidationStartTrue)
    {
        m_currentScreenType = ScreenType::TerminalCapabilities;
    }
    else
    {
        m_currentScreenType = ScreenType::ControlDeck;
    }

    m_screenCycleElapsedSeconds = 0.0;

    rebuildNavigationButtons();

    return replaceWith(routeIdForScreenType(m_currentScreenType));
}

bool DemoApplication::dispatchEvent(const Input::Event& event)
{
    if (m_navigationButtons.handleEvent(event))
    {
        return true;
    }

    return ApplicationHost::dispatchEvent(event);
}

bool DemoApplication::handleApplicationCommand(const Input::CommandEvent& commandEvent)
{
    switch (commandEvent.command.code)
    {
    case Input::CommandCode::Forward:
        advanceScreen();
        return true;

    case Input::CommandCode::Back:
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

    m_navigationButtons.update(event);
}

void DemoApplication::render()
{
    surface().clear(Themes::Disabled);

    screenManager().drawCurrentScreen(surface());

    positionNavigationButtons();
    m_navigationButtons.draw(surface());

    renderer().present(surface().buffer());
}

void DemoApplication::registerDemoRoutes()
{
    registerScreenRoute(
        routeIdForScreenType(ScreenType::TerminalCapabilities),
        [this]() -> std::unique_ptr<Screen>
        {
            return std::make_unique<TerminalCapabilitiesScreen>(&renderer());
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::RendererDiagnostics),
        [this]() -> std::unique_ptr<Screen>
        {
            return std::make_unique<RendererDiagnosticsScreen>(&renderer());
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::ControlDeck),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<ControlDeckScreen>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::RetroTerminal),
        [this]() -> std::unique_ptr<Screen>
        {
            return std::make_unique<RetroTerminalScreen>(assetLibrary());
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::NeonDialog),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<NeonDialogScreen>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::OpsWall),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<OpsWallScreen>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::MenuDemoScreen),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<MenuDemoScreen>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::WidgetDemoScreen),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<WidgetDemoScreen>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::ScrollableTileGridDemo),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<ScrollableTileGridDemoScreen>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::WindowDemo),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<WindowDemo>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::DigitalRain),
        [this]() -> std::unique_ptr<Screen>
        {
            return std::make_unique<DigitalRainScreen>(startupDiagnostics().actualHost);
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::WaterEffect),
        [this]() -> std::unique_ptr<Screen>
        {
            return std::make_unique<WaterEffectScreen>(assetLibrary());
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::Donut3D),
        []() -> std::unique_ptr<Screen>
        {
            return std::make_unique<Donut3DScreen>();
        });

    registerScreenRoute(
        routeIdForScreenType(ScreenType::Fire),
        [this]() -> std::unique_ptr<Screen>
        {
            return std::make_unique<FireScreen>(assetLibrary());
        });
}

void DemoApplication::switchToScreen(ScreenType screenType)
{
    m_currentScreenType = screenType;
    m_screenCycleElapsedSeconds = 0.0;

    rebuildNavigationButtons();

    replaceWith(routeIdForScreenType(screenType));
}

void DemoApplication::advanceScreen()
{
    switchToScreen(nextScreenType(m_currentScreenType));
}

void DemoApplication::previousScreen()
{
    switchToScreen(previousScreenType(m_currentScreenType));
}

DemoApplication::ScreenType DemoApplication::nextScreenType(ScreenType screenType) const
{
    switch (screenType)
    {
    case ScreenType::ControlDeck:
        return ScreenType::RetroTerminal;

    case ScreenType::RetroTerminal:
        return ScreenType::NeonDialog;

    case ScreenType::NeonDialog:
        return ScreenType::OpsWall;

    case ScreenType::OpsWall:
        return ScreenType::MenuDemoScreen;

    case ScreenType::MenuDemoScreen:
        return ScreenType::WidgetDemoScreen;

    case ScreenType::WidgetDemoScreen:
        return ScreenType::ScrollableTileGridDemo;

    case ScreenType::ScrollableTileGridDemo:
        return ScreenType::WindowDemo;

    case ScreenType::WindowDemo:
        return ScreenType::DigitalRain;

    case ScreenType::DigitalRain:
        return ScreenType::WaterEffect;

    case ScreenType::WaterEffect:
        return ScreenType::Donut3D;

    case ScreenType::Donut3D:
        return ScreenType::Fire;

    case ScreenType::Fire:
        return ScreenType::ControlDeck;

    case ScreenType::TerminalCapabilities:
        return ScreenType::RendererDiagnostics;

    case ScreenType::RendererDiagnostics:
        return ScreenType::TerminalCapabilities;
    }

    return ScreenType::ControlDeck;
}

DemoApplication::ScreenType DemoApplication::previousScreenType(ScreenType screenType) const
{
    switch (screenType)
    {
    case ScreenType::ControlDeck:
        return ScreenType::Fire;

    case ScreenType::RetroTerminal:
        return ScreenType::ControlDeck;

    case ScreenType::NeonDialog:
        return ScreenType::RetroTerminal;

    case ScreenType::OpsWall:
        return ScreenType::NeonDialog;

    case ScreenType::MenuDemoScreen:
        return ScreenType::OpsWall;

    case ScreenType::WidgetDemoScreen:
        return ScreenType::MenuDemoScreen;

    case ScreenType::ScrollableTileGridDemo:
        return ScreenType::WidgetDemoScreen;

    case ScreenType::WindowDemo:
        return ScreenType::ScrollableTileGridDemo;

    case ScreenType::DigitalRain:
        return ScreenType::WindowDemo;

    case ScreenType::WaterEffect:
        return ScreenType::DigitalRain;

    case ScreenType::Donut3D:
        return ScreenType::WaterEffect;

    case ScreenType::Fire:
        return ScreenType::Donut3D;

    case ScreenType::TerminalCapabilities:
        return ScreenType::RendererDiagnostics;

    case ScreenType::RendererDiagnostics:
        return ScreenType::TerminalCapabilities;
    }

    return ScreenType::ControlDeck;
}

std::string DemoApplication::routeIdForScreenType(ScreenType screenType) const
{
    switch (screenType)
    {
    case ScreenType::TerminalCapabilities:
        return "demo:terminal_capabilities";

    case ScreenType::RendererDiagnostics:
        return "demo:renderer_diagnostics";

    case ScreenType::ControlDeck:
        return "demo:control_deck";

    case ScreenType::RetroTerminal:
        return "demo:retro_terminal";

    case ScreenType::NeonDialog:
        return "demo:neon_dialog";

    case ScreenType::OpsWall:
        return "demo:ops_wall";

    case ScreenType::MenuDemoScreen:
        return "demo:menu_demo";

    case ScreenType::WidgetDemoScreen:
        return "demo:widget_demo";

    case ScreenType::ScrollableTileGridDemo:
        return "demo:scrollable_tile_grid";

    case ScreenType::WindowDemo:
        return "demo:window_demo";

    case ScreenType::DigitalRain:
        return "demo:digital_rain";

    case ScreenType::WaterEffect:
        return "demo:water_effect";

    case ScreenType::Donut3D:
        return "demo:donut_3d";

    case ScreenType::Fire:
        return "demo:fire";
    }

    return "demo:control_deck";
}

void DemoApplication::rebuildNavigationButtons()
{
    m_navigationButtons.clearChildren();

    auto previousButton = std::make_unique<Button>(
        Rect{ Point{ 0, 0 }, Size{ 14, 3 } },
        "Previous",
        "route:" + routeIdForScreenType(previousScreenType(m_currentScreenType)));

    previousButton->setActivationCallback(
        [this](const ButtonActivationResult& result)
        {
            if (!result.activated)
            {
                return;
            }

            dispatchNavigationCommandId(result.commandId);
        });

    auto nextButton = std::make_unique<Button>(
        Rect{ Point{ 0, 0 }, Size{ 10, 3 } },
        "Next",
        "route:" + routeIdForScreenType(nextScreenType(m_currentScreenType)));

    nextButton->setActivationCallback(
        [this](const ButtonActivationResult& result)
        {
            if (!result.activated)
            {
                return;
            }

            dispatchNavigationCommandId(result.commandId);
        });

    m_navigationButtons.addChild(std::move(previousButton));
    m_navigationButtons.addChild(std::move(nextButton));

    positionNavigationButtons();
}

void DemoApplication::positionNavigationButtons()
{
    const int buttonY = 2;
    const int nextWidth = 10;
    const int previousWidth = 14;
    const int gap = 2;
    const int rightPadding = 4;

    const int nextX = width() - rightPadding - nextWidth;
    const int previousX = nextX - gap - previousWidth;

    m_navigationButtons.setBounds(
        Rect{
            Point{ previousX, buttonY },
            Size{ previousWidth + gap + nextWidth, 3 }
        });

    if (Widget* previousButton = m_navigationButtons.childAt(0))
    {
        previousButton->setBounds(
            Rect{ Point{ previousX, buttonY }, Size{ previousWidth, 3 } });
    }

    if (Widget* nextButton = m_navigationButtons.childAt(1))
    {
        nextButton->setBounds(
            Rect{ Point{ nextX, buttonY }, Size{ nextWidth, 3 } });
    }
}

bool DemoApplication::dispatchNavigationCommandId(const std::string& commandId)
{
    if (commandId.empty())
    {
        return false;
    }

    if (commandId == "route:" + routeIdForScreenType(nextScreenType(m_currentScreenType)))
    {
        m_currentScreenType = nextScreenType(m_currentScreenType);
        m_screenCycleElapsedSeconds = 0.0;

        const bool handled = handleCommandId(commandId);

        rebuildNavigationButtons();

        return handled;
    }

    if (commandId == "route:" + routeIdForScreenType(previousScreenType(m_currentScreenType)))
    {
        m_currentScreenType = previousScreenType(m_currentScreenType);
        m_screenCycleElapsedSeconds = 0.0;

        const bool handled = handleCommandId(commandId);

        rebuildNavigationButtons();

        return handled;
    }

    return handleCommandId(commandId);
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
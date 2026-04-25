#include "App/Application.h"
#include "App/ScreenManager.h"
#include "Input/InputManager.h"

#include <chrono>
#include <thread>
#include <vector>
#include <windows.h>

#include "Input/Event.h"
#include "Rendering/ConsoleRenderer.h"
#include "Rendering/Surface.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/TerminalRenderer.h"

#include "Screens/ShowcaseScreen.h"
#include "Screens/DigitalRainScreen.h"
#include "Screens/WaterEffectScreen.h"
#include "Screens/Donut3DScreen.h"
#include "Screens/FireScreen.h"
#include "Screens/Developer/RendererDiagnosticsScreen.h"
#include "Screens/Developer/TerminalCapabilitiesScreen.h"

static Application* g_appInstance = nullptr;

Application::Application(
    StartupRendererSelection rendererSelection,
    StartupValidationScreenPreference validationScreenSelection,
    const StartupDiagnosticsContext& startupDiagnostics)
    : m_rendererSelection(rendererSelection)
    , m_validationScreenStart(validationScreenSelection)
    , m_startupDiagnostics(startupDiagnostics)
{
}

Application::~Application()
{
    shutdown();
}

#define NOMINMAX
#include <windows.h>

namespace
{
    void maximizeHostWindow()
    {
        HWND hwnd = GetConsoleWindow();
        if (hwnd != nullptr)
        {
            ShowWindow(hwnd, SW_MAXIMIZE);
        }
    }
}

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    case CTRL_LOGOFF_EVENT:
    {
        if (g_appInstance != nullptr)
        {
            g_appInstance->shutdown();
        }

        return TRUE;
    }
    default:
        return FALSE;
    }
}

bool Application::initialize()
{
    g_appInstance = this;
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    if (m_rendererSelection == StartupRendererSelection::Terminal)
    {
        auto terminalRenderer = std::make_unique<TerminalRenderer>();
        terminalRenderer->setDiagnosticsEnabled(true);
        terminalRenderer->setDiagnosticsAppendMode(false);
        terminalRenderer->setDiagnosticsOutputPath("render_diagnostics_report.txt");

        StartupDiagnosticsContext terminalDiagnostics = m_startupDiagnostics;
        terminalDiagnostics.actualRenderer = RendererKind::TerminalRenderer;
        terminalRenderer->setStartupDiagnosticsContext(terminalDiagnostics);

        if (terminalRenderer->initialize())
        {
            m_startupDiagnostics = terminalDiagnostics;
            m_renderer = std::move(terminalRenderer);
        }
    }

    if (!m_renderer)
    {
        auto consoleRenderer = std::make_unique<ConsoleRenderer>();
        consoleRenderer->setDiagnosticsEnabled(true);
        consoleRenderer->setDiagnosticsAppendMode(false);
        consoleRenderer->setDiagnosticsOutputPath("render_diagnostics_report.txt");

        StartupDiagnosticsContext consoleDiagnostics = m_startupDiagnostics;
        consoleDiagnostics.actualRenderer = RendererKind::ConsoleRenderer;
        consoleRenderer->setStartupDiagnosticsContext(consoleDiagnostics);

        if (!consoleRenderer->initialize())
        {
            return false;
        }

        m_startupDiagnostics = consoleDiagnostics;
        m_renderer = std::move(consoleRenderer);
    }

    if (m_startupDiagnostics.actualHost == TerminalHostKind::ClassicConsoleWindow)
    {
        maximizeHostWindow();
    }

    m_width = m_renderer->getConsoleWidth();
    m_height = m_renderer->getConsoleHeight();

    m_surface = std::make_unique<Surface>(m_width, m_height);
    m_screenManager = std::make_unique<ScreenManager>();

    m_inputManager = std::make_unique<Input::InputManager>();
    
    if (!m_inputManager->initialize())
    {
        return false;
    }

    m_commandMap = Input::CommandMap::createDefault();

    configureAssetLibrary();

    if (m_validationScreenStart == StartupValidationScreenPreference::ValidationStartTrue)
    {
        m_currentScreenType = ScreenType::TerminalCapabilities;
    }
    else
    {
        // Start Sccren Selection
        m_currentScreenType = ScreenType::ControlDeck;
    }
    m_screenCycleElapsedSeconds = 0.0;

    switchToScreen(m_currentScreenType);

    m_running = true;
    return true;
}

bool Application::dispatchEvent(const Input::Event& event)
{
    if (dispatchToActiveScreen(event))
    {
        return true;
    }

    if (const Input::CommandEvent* commandEvent = event.asCommand())
    {
        return handleApplicationCommand(*commandEvent);
    }

    return false;
}

bool Application::dispatchToActiveScreen(const Input::Event& event)
{
    if (!m_screenManager)
    {
        return false;
    }

    return m_screenManager->dispatchEvent(event);
}

bool Application::handleApplicationCommand(const Input::CommandEvent& commandEvent)
{
    switch (commandEvent.command.code)
    {
    case Input::CommandCode::Quit:
    case Input::CommandCode::Close:
        shutdown();
        return true;

    case Input::CommandCode::Confirm:
    case Input::CommandCode::Forward:
        advanceScreen();
        return true;

    case Input::CommandCode::Back:
        // Future tier: route to previous screen/page history.
        return false;

    case Input::CommandCode::Refresh:
        render();
        return true;

    default:
        return false;
    }
}

void Application::run()
{
    using clock = std::chrono::high_resolution_clock;

    auto previousTime = clock::now();

    while (m_running)
    {
        auto currentTime = clock::now();
        std::chrono::duration<double> delta = currentTime - previousTime;
        previousTime = currentTime;

        handleResize();

        if (m_inputManager)
        {
            m_inputManager->poll();

            std::vector<Input::Event> events;

            for (const Input::KeyEvent& keyEvent : m_inputManager->drainEvents())
            {
                events.push_back(Input::Event::key(keyEvent));

                std::optional<Input::Command> command = m_commandMap.map(keyEvent);
                if (command.has_value())
                {
                    events.push_back(Input::Event::command(command.value()));
                }
            }

            events.push_back(Input::Event::tick(delta.count(), m_frameIndex));

            for (const Input::Event& event : events)
            {
                dispatchEvent(event);

                if (!m_running)
                {
                    break;
                }
            }
        }

        if (!m_running)
        {
            break;
        }

        update(delta.count());
        render();

        ++m_frameIndex;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void Application::shutdown()
{
    if (!m_running)
    {
        return;
    }

    m_running = false;

    m_screenManager.reset();

    if (m_renderer)
    {
        m_renderer->shutdown();
    }

    if (m_inputManager)
    {
        m_inputManager->shutdown();
        m_inputManager.reset();
    }
}

void Application::handleResize()
{
    if (m_renderer->pollResize())
    {
        const Size previousSize{ m_width, m_height };

        m_width = m_renderer->getConsoleWidth();
        m_height = m_renderer->getConsoleHeight();

        const Size currentSize{ m_width, m_height };

        m_surface->resize(m_width, m_height);

        Input::Event resizeEvent = Input::Event::resize(previousSize, currentSize);
        dispatchEvent(resizeEvent);
    }
}

void Application::update(double deltaTime)
{
    updateScreenCycle(deltaTime);
    m_screenManager->update(deltaTime);
}

void Application::render()
{
    m_surface->clear(Themes::Disabled);

    m_screenManager->drawCurrentScreen(*m_surface);

    m_renderer->present(m_surface->buffer());
}

void Application::switchToScreen(ScreenType screenType)
{
    m_screenManager->clearScreens();

    switch (screenType)
    {
    case ScreenType::TerminalCapabilities:
        m_screenManager->pushScreen(std::make_unique<TerminalCapabilitiesScreen>(m_renderer.get()));
        break;

    case ScreenType::RendererDiagnostics:
        m_screenManager->pushScreen(std::make_unique<RendererDiagnosticsScreen>(m_renderer.get()));
        break;

    case ScreenType::ControlDeck:
        m_screenManager->pushScreen(std::make_unique<ControlDeckScreen>());
        break;

    case ScreenType::RetroTerminal:
        m_screenManager->pushScreen(std::make_unique<RetroTerminalScreen>(m_assetLibrary));        break;

    case ScreenType::NeonDialog:
        m_screenManager->pushScreen(std::make_unique<NeonDialogScreen>());
        break;

    case ScreenType::OpsWall:
        m_screenManager->pushScreen(std::make_unique<OpsWallScreen>());
        break;

    case ScreenType::DigitalRain:
        m_screenManager->pushScreen(std::make_unique<DigitalRainScreen>(m_startupDiagnostics.actualHost));
        break;

    case ScreenType::WaterEffect:
        m_screenManager->pushScreen(std::make_unique<WaterEffectScreen>(m_assetLibrary));
        break;

    case ScreenType::Donut3D:
        m_screenManager->pushScreen(std::make_unique<Donut3DScreen>());
        break;

    case ScreenType::Fire:
        m_screenManager->pushScreen(std::make_unique<FireScreen>(m_assetLibrary));
        break;
    }

    m_currentScreenType = screenType;
}

void Application::advanceScreen()
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

void Application::updateScreenCycle(double deltaTime)
{
    m_screenCycleElapsedSeconds += deltaTime;

    while (m_screenCycleElapsedSeconds >= m_screenCycleIntervalSeconds)
    {
        m_screenCycleElapsedSeconds -= m_screenCycleIntervalSeconds;
        advanceScreen();
    }
}

void Application::configureAssetLibrary()
{
    m_assetLibrary.setAssetsRoot("Assets");

    m_assetLibrary.registerAsset(
        "banner.fire_font_k",
        "Fonts/FIGlet/Fire Font-k.flf");

    m_assetLibrary.registerAsset(
        "banner.ansi_shadow",
        "Fonts/FIGlet/ANSI Shadow.flf");

    m_assetLibrary.registerAsset(
        "banner.computer",
        "Fonts/Figlet/Computer.flf");

    m_assetLibrary.registerAsset(
        "pfont.assemble_box",
        "Fonts/pFont/AssembleBox.pfont");

    m_assetLibrary.registerAsset(
        "xp.retro_terminal_1",
        "xp/retroTerm1.xp");
}

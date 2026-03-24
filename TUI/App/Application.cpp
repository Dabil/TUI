#include "App/Application.h"
#include "App/ScreenManager.h"

#include <chrono>
#include <thread>

#include "Rendering/ConsoleRenderer.h"
#include "Rendering/Surface.h"
#include "Rendering/Styles/Themes.h"

#include "Screens/Donut3DScreen.h"
#include "Screens/WaterEffectScreen.h"

Application::Application() = default;

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{
    m_renderer = std::make_unique<ConsoleRenderer>();

    if (!m_renderer->initialize())
    {
        return false;
    }

    m_width = m_renderer->getConsoleWidth();
    m_height = m_renderer->getConsoleHeight();

    m_surface = std::make_unique<Surface>(m_width, m_height);
    m_screenManager = std::make_unique<ScreenManager>();

    m_currentScreenType = ScreenType::WaterEffect;
    m_screenCycleElapsedSeconds = 0.0;

    switchToScreen(m_currentScreenType);

    m_running = true;
    return true;
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

        update(delta.count());
        render();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void Application::shutdown()
{
    if (m_renderer)
    {
        m_renderer->shutdown();
    }
}

void Application::handleResize()
{
    if (m_renderer->pollResize())
    {
        m_width = m_renderer->getConsoleWidth();
        m_height = m_renderer->getConsoleHeight();

        m_surface->resize(m_width, m_height);
    }
}

void Application::update(double deltaTime)
{
    updateScreenCycle(deltaTime);
    m_screenManager->update(deltaTime);
}

void Application::render()
{
    m_surface->clear(Themes::AccentSurface);

    m_screenManager->drawCurrentScreen(*m_surface);

    m_renderer->present(m_surface->buffer());
}

void Application::switchToScreen(ScreenType screenType)
{
    m_screenManager->clearScreens();

    switch (screenType)
    {
    case ScreenType::WaterEffect:
        m_screenManager->pushScreen(std::make_unique<WaterEffectScreen>());
        break;

    case ScreenType::Donut3D:
        m_screenManager->pushScreen(std::make_unique<Donut3DScreen>());
        break;
    }

    m_currentScreenType = screenType;
}

void Application::advanceScreen()
{
    switch (m_currentScreenType)
    {
    case ScreenType::WaterEffect:
        switchToScreen(ScreenType::Donut3D);
        break;

    case ScreenType::Donut3D:
        switchToScreen(ScreenType::Starfield);
        break;

    case ScreenType::Starfield:
        switchToScreen(ScreenType::WaterEffect);
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
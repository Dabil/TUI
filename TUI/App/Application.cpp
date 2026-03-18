#include "APP/Application.h"

#include <thread>
#include <chrono>

#include "Rendering/ConsoleRenderer.h"
#include "App/ScreenManager.h"
#include "Rendering/Surface.h"
#include "Screens/ShowcaseScreen.h"

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

    m_screenManager->pushScreen(std::make_unique<ShowcaseScreen>());

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
    m_screenManager->update(deltaTime);
}

void Application::render()
{

}

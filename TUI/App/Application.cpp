#include "APP/Application.h"

#include "Rendering/ConsoleRenderer.h"

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

}

void Application::update(double deltaTime)
{

}

void Application::render()
{

}

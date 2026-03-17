#include "APP/Application.h"

#include "Rendering/ConsoleRenderer.h"

Application::Application() = default;

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{

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

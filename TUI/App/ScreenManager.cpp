#include "App/ScreenManager.h"

#include "Screens/Screen.h"
#include "Rendering/Surface.h"

void ScreenManager::pushScreen(std::unique_ptr<Screen> screen)
{
    if (screen)
    {
        screen->onEnter();
        m_screenStack.push_back(std::move(screen));
    }
}

void ScreenManager::popScreen()
{
    if (!m_screenStack.empty())
    {
        m_screenStack.back()->onExit();
        m_screenStack.pop_back();
    }
}

void ScreenManager::clearScreens()
{
    while (!m_screenStack.empty())
    {
        popScreen();
    }
}

bool ScreenManager::hasScreens() const
{
    return !m_screenStack.empty();
}

Screen* ScreenManager::currentScreen()
{

}

void ScreenManager::update(double deltaTime)
{

}

void ScreenManager::drawCurrentScreen(Surface& surface)
{

}

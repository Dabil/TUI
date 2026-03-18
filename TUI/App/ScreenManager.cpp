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

}

void ScreenManager::clearScreens()
{

}

bool ScreenManager::hasScreens() const
{

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

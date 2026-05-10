#include "UI/Panels/EffectWindow.h"

#include <utility>

#include "Core/Rect.h"
#include "Rendering/Surface.h"

EffectWindow::EffectWindow(Rect bounds, std::string title, IEffect& effect)
    : Window(bounds, std::move(title))
    , m_effect(effect)
{
}

void EffectWindow::update(const Animation::TickEvent& event)
{
    if (!isVisible() || !isEnabled())
    {
        return;
    }

    m_effect.update(event);
}

void EffectWindow::draw(Surface& surface)
{
    if (!isVisible())
    {
        return;
    }

    Window::draw(surface);

    m_effect.draw(surface, contentBounds());
}
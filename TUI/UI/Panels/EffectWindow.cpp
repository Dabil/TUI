#include "UI/Panels/EffectWindow.h"

#include <memory>
#include <utility>

#include "Core/Rect.h"
#include "Rendering/Surface.h"
#include "UI/Content/EffectReferenceWindowContent.h"

EffectWindow::EffectWindow(Rect bounds, std::string title, IEffect& effect)
    : Window(bounds, std::move(title))
    , m_effect(effect)
{
}


bool EffectWindow::hasTransferableContent() const
{
    return true;
}

std::unique_ptr<UI::IWindowContent> EffectWindow::releaseContent()
{
    return std::make_unique<UI::EffectReferenceWindowContent>(m_effect);
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
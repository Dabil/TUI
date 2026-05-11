#include "UI/Panels/ContentWindow.h"

#include <utility>

#include "Input/Event.h"
#include "Rendering/Surface.h"

namespace UI
{
    ContentWindow::ContentWindow(
        const Rect& bounds,
        std::string title,
        std::unique_ptr<IWindowContent> content)
        : Window(bounds, std::move(title))
        , m_content(std::move(content))
    {
        if (m_content != nullptr)
        {
            m_content->onAttached();
            notifyContentBoundsChanged();
        }
    }

    bool ContentWindow::hasTransferableContent() const
    {
        return m_content != nullptr;
    }

    std::unique_ptr<IWindowContent> ContentWindow::releaseContent()
    {
        if (m_content != nullptr)
        {
            m_content->onDetached();
        }

        return std::move(m_content);
    }

    void ContentWindow::update(const Animation::TickEvent& event)
    {
        if (!isVisible() || !isEnabled())
        {
            return;
        }

        notifyContentBoundsChanged();

        if (m_content != nullptr)
        {
            m_content->update(event);
        }
    }

    bool ContentWindow::handleEvent(const Input::Event& event)
    {
        if (m_content != nullptr && m_content->handleEvent(event))
        {
            return true;
        }

        return Window::handleEvent(event);
    }

    void ContentWindow::draw(Surface& surface)
    {
        if (!isVisible())
        {
            return;
        }

        notifyContentBoundsChanged();
        Window::draw(surface);

        if (m_content != nullptr)
        {
            m_content->draw(surface, contentBounds());
        }
    }

    void ContentWindow::notifyContentBoundsChanged()
    {
        if (m_content == nullptr)
        {
            return;
        }

        const Rect currentBounds = contentBounds();

        if (currentBounds.position.x == m_lastContentBounds.position.x &&
            currentBounds.position.y == m_lastContentBounds.position.y &&
            currentBounds.size.width == m_lastContentBounds.size.width &&
            currentBounds.size.height == m_lastContentBounds.size.height)
        {
            return;
        }

        m_lastContentBounds = currentBounds;
        m_content->onBoundsChanged(currentBounds);
    }
}


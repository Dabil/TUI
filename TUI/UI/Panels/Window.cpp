#include "UI/Panels/Window.h"

#include <utility>

Window::Window()
    : Panel()
{
}

Window::Window(const Rect& bounds)
    : Panel(bounds)
{
}

Window::Window(const Rect& bounds, std::string title)
    : Panel(bounds, std::move(title))
{
}

Window::Window(const Rect& bounds, std::string title, bool modal)
    : Panel(bounds, std::move(title))
    , m_modal(modal)
{
}

bool Window::isModal() const
{
    return m_modal;
}

void Window::setModal(bool modal)
{
    m_modal = modal;
}
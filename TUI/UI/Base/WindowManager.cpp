#include "UI/Base/WindowManager.h"

#include <algorithm>
#include <utility>

#include "Core/Rect.h"
#include "UI/Panels/Window.h"

WindowManager::WindowManager() = default;

WindowManager::~WindowManager() = default;

void WindowManager::addWindow(Window& window)
{
    if (contains(window))
    {
        return;
    }

    m_windows.push_back({
        &window,
        nextFrontZOrder(),
        m_nextInsertionOrder++
        });

    sortWindows();
}

void WindowManager::removeWindow(Window& window)
{
    m_windows.erase(
        std::remove_if(
            m_windows.begin(),
            m_windows.end(),
            [&window](const ManagedWindow& entry)
            {
                return entry.window == &window;
            }),
        m_windows.end());
}

void WindowManager::clear()
{
    m_windows.clear();
}

bool WindowManager::contains(const Window& window) const
{
    return findEntry(window) != nullptr;
}

std::size_t WindowManager::count() const
{
    return m_windows.size();
}

bool WindowManager::empty() const
{
    return m_windows.empty();
}

void WindowManager::show(Window& window)
{
    if (!contains(window))
    {
        addWindow(window);
    }

    window.show();
}

void WindowManager::hide(Window& window)
{
    if (ManagedWindow* entry = findEntry(window))
    {
        entry->window->hide();
    }
}

void WindowManager::bringToFront(Window& window)
{
    ManagedWindow* entry = findEntry(window);

    if (entry == nullptr)
    {
        addWindow(window);
        return;
    }

    entry->zOrder = nextFrontZOrder();
    entry->insertionOrder = m_nextInsertionOrder++;

    sortWindows();
}

void WindowManager::sendToBack(Window& window)
{
    ManagedWindow* entry = findEntry(window);

    if (entry == nullptr)
    {
        addWindow(window);
        entry = findEntry(window);
    }

    if (entry == nullptr)
    {
        return;
    }

    entry->zOrder = nextBackZOrder();
    entry->insertionOrder = m_nextInsertionOrder++;

    sortWindows();
}

void WindowManager::setZOrder(Window& window, int zOrder)
{
    ManagedWindow* entry = findEntry(window);

    if (entry == nullptr)
    {
        addWindow(window);
        entry = findEntry(window);
    }

    if (entry == nullptr)
    {
        return;
    }

    entry->zOrder = zOrder;
    entry->insertionOrder = m_nextInsertionOrder++;

    sortWindows();
}

int WindowManager::zOrderOf(const Window& window) const
{
    const ManagedWindow* entry = findEntry(window);
    return entry != nullptr ? entry->zOrder : 0;
}

Window* WindowManager::topWindow()
{
    for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it)
    {
        if (it->window != nullptr && it->window->isVisible())
        {
            return it->window;
        }
    }

    return nullptr;
}

const Window* WindowManager::topWindow() const
{
    for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it)
    {
        if (it->window != nullptr && it->window->isVisible())
        {
            return it->window;
        }
    }

    return nullptr;
}

Window* WindowManager::topModalWindow()
{
    for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it)
    {
        if (it->window != nullptr &&
            it->window->isVisible() &&
            it->window->isModal())
        {
            return it->window;
        }
    }

    return nullptr;
}

const Window* WindowManager::topModalWindow() const
{
    for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it)
    {
        if (it->window != nullptr &&
            it->window->isVisible() &&
            it->window->isModal())
        {
            return it->window;
        }
    }

    return nullptr;
}

bool WindowManager::hasModalWindow() const
{
    return topModalWindow() != nullptr;
}

bool WindowManager::canRouteTo(const Window& window) const
{
    if (!window.isVisible() || !window.isEnabled())
    {
        return false;
    }

    const ManagedWindow* targetEntry = findEntry(window);

    if (targetEntry == nullptr)
    {
        return false;
    }

    const Window* modalWindow = topModalWindow();

    if (modalWindow == nullptr)
    {
        return true;
    }

    const ManagedWindow* modalEntry = findEntry(*modalWindow);

    if (modalEntry == nullptr)
    {
        return true;
    }

    return targetEntry->zOrder >= modalEntry->zOrder;
}

void WindowManager::update(double deltaTime)
{
    for (ManagedWindow& entry : m_windows)
    {
        if (entry.window == nullptr)
        {
            continue;
        }

        if (!canRouteTo(*entry.window))
        {
            continue;
        }

        entry.window->update(deltaTime);
    }
}

void WindowManager::draw(Surface& surface)
{
    sortWindows();

    for (ManagedWindow& entry : m_windows)
    {
        if (entry.window == nullptr || !entry.window->isVisible())
        {
            continue;
        }

        entry.window->draw(surface);
    }
}

std::vector<LayerInstance> WindowManager::buildLayers()
{
    sortWindows();

    std::vector<LayerInstance> layers;
    layers.reserve(m_windows.size());

    for (ManagedWindow& entry : m_windows)
    {
        Window* window = entry.window;

        if (window == nullptr || !window->isVisible())
        {
            continue;
        }

        const Rect originalBounds = window->bounds();

        if (originalBounds.size.width <= 0 || originalBounds.size.height <= 0)
        {
            continue;
        }

        Surface layerSurface(
            originalBounds.size.width,
            originalBounds.size.height);

        window->setPosition(0, 0);
        window->draw(layerSurface);
        window->setBounds(originalBounds);

        layers.emplace_back(
            std::move(layerSurface),
            originalBounds.position,
            entry.zOrder,
            true);
    }

    return layers;
}

WindowManager::ManagedWindow* WindowManager::findEntry(Window& window)
{
    auto it = std::find_if(
        m_windows.begin(),
        m_windows.end(),
        [&window](const ManagedWindow& entry)
        {
            return entry.window == &window;
        });

    return it != m_windows.end() ? &(*it) : nullptr;
}

const WindowManager::ManagedWindow* WindowManager::findEntry(const Window& window) const
{
    auto it = std::find_if(
        m_windows.begin(),
        m_windows.end(),
        [&window](const ManagedWindow& entry)
        {
            return entry.window == &window;
        });

    return it != m_windows.end() ? &(*it) : nullptr;
}

void WindowManager::sortWindows()
{
    std::stable_sort(
        m_windows.begin(),
        m_windows.end(),
        [](const ManagedWindow& lhs, const ManagedWindow& rhs)
        {
            if (lhs.zOrder != rhs.zOrder)
            {
                return lhs.zOrder < rhs.zOrder;
            }

            return lhs.insertionOrder < rhs.insertionOrder;
        });
}

int WindowManager::nextFrontZOrder() const
{
    if (m_windows.empty())
    {
        return 0;
    }

    const auto it = std::max_element(
        m_windows.begin(),
        m_windows.end(),
        [](const ManagedWindow& lhs, const ManagedWindow& rhs)
        {
            return lhs.zOrder < rhs.zOrder;
        });

    return it->zOrder + 1;
}

int WindowManager::nextBackZOrder() const
{
    if (m_windows.empty())
    {
        return 0;
    }

    const auto it = std::min_element(
        m_windows.begin(),
        m_windows.end(),
        [](const ManagedWindow& lhs, const ManagedWindow& rhs)
        {
            return lhs.zOrder < rhs.zOrder;
        });

    return it->zOrder - 1;
}
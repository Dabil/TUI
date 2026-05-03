#pragma once

#include <cstddef>
#include <vector>

#include "Rendering/LayerInstance.h"
#include "Rendering/Surface.h"

class Window;

class WindowManager
{
public:
    WindowManager();
    ~WindowManager();

    void addWindow(Window& window);
    void removeWindow(Window& window);
    void clear();

    bool contains(const Window& window) const;
    std::size_t count() const;
    bool empty() const;

    void show(Window& window);
    void hide(Window& window);

    void bringToFront(Window& window);
    void sendToBack(Window& window);

    void setZOrder(Window& window, int zOrder);
    int zOrderOf(const Window& window) const;

    Window* topWindow();
    const Window* topWindow() const;

    Window* topModalWindow();
    const Window* topModalWindow() const;

    bool hasModalWindow() const;
    bool canRouteTo(const Window& window) const;

    void update(double deltaTime);
    void draw(Surface& surface);

    std::vector<LayerInstance> buildLayers();

private:
    struct ManagedWindow
    {
        Window* window = nullptr;
        int zOrder = 0;
        std::size_t insertionOrder = 0;
    };

private:
    ManagedWindow* findEntry(Window& window);
    const ManagedWindow* findEntry(const Window& window) const;

    void sortWindows();
    int nextFrontZOrder() const;
    int nextBackZOrder() const;

private:
    std::vector<ManagedWindow> m_windows;
    std::size_t m_nextInsertionOrder = 0;
};
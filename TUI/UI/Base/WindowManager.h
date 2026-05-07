// UI/Base/WindowManager.h
#pragma once

#include <cstddef>
#include <vector>

#include "Core/Point.h"
#include "Rendering/LayerInstance.h"
#include "Rendering/Surface.h"
#include "UI/Interaction/WindowInteraction.h"
#include "Input/MouseEvent.h"

namespace Input
{
    class Event;
    struct MouseEvent;
}

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

    Window* hoveredWindow();
    const Window* hoveredWindow() const;

    Window* focusedWindow();
    const Window* focusedWindow() const;

    bool hasModalWindow() const;
    bool canRouteTo(const Window& window) const;
    bool handleEvent(const Input::Event& event);
    bool handleMouseEvent(const Input::MouseEvent& mouseEvent);

    UI::WindowHitTestResult hitTest(Point screenPosition);
    UI::WindowHitTestResult hitTest(Point screenPosition) const;

    void capturePointer(Window& window, UI::PointerButton button, Point screenPosition);
    void releasePointer(Window& window);
    void releasePointer();
    bool hasPointerCapture() const;
    const UI::PointerCaptureState& pointerCapture() const;
    Window* capturedWindow();
    const Window* capturedWindow() const;

    bool beginDrag(Window& window, Point screenPosition, UI::PointerButton button = UI::PointerButton::Primary);
    bool beginDragAt(Point screenPosition, UI::PointerButton button = UI::PointerButton::Primary);
    bool updateDrag(Point screenPosition);
    void endDrag();
    bool isDragging() const;
    const UI::WindowDragState& dragState() const;

    bool beginResize(Window& window, UI::CursorRegion region, Point screenPosition, UI::PointerButton button = UI::PointerButton::Primary);
    bool beginResizeAt(Point screenPosition, UI::PointerButton button = UI::PointerButton::Primary);
    bool updateResize(Point screenPosition);
    void endResize();
    bool isResizing() const;
    const UI::WindowResizeState& resizeState() const;

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

    void setHoveredWindow(Window* window);
    void setFocusedWindow(Window* window);

private:
    std::vector<ManagedWindow> m_windows;
    std::size_t m_nextInsertionOrder = 0;

    UI::PointerCaptureState m_pointerCapture;
    UI::WindowDragState m_dragState;
    UI::WindowResizeState m_resizeState;

    Window* m_hoveredWindow = nullptr;
    Window* m_focusedWindow = nullptr;
};
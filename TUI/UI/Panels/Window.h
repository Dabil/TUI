#pragma once

#include <string>

#include "Core/Point.h"
#include "Core/Rect.h"
#include "UI/Interaction/WindowInteraction.h"
#include "UI/Panels/Panel.h"

class Window : public Panel
{
public:
    Window();
    explicit Window(const Rect& bounds);
    Window(const Rect& bounds, std::string title);
    Window(const Rect& bounds, std::string title, bool modal);

    bool isModal() const;
    void setModal(bool modal);

    bool isDraggable() const;
    void setDraggable(bool draggable);

    bool isResizable() const;
    void setResizable(bool resizable);

    Size minimumSize() const;
    void setMinimumSize(Size size);
    void setMinimumSize(int width, int height);

    int resizeBorderThickness() const;
    void setResizeBorderThickness(int thickness);

    int titleBarHeight() const;
    void setTitleBarHeight(int height);

    UI::CursorRegion hitTest(Point screenPosition) const;
    bool containsScreenPoint(Point screenPosition) const;

private:
    bool m_modal = false;
    bool m_draggable = true;
    bool m_resizable = true;
    Size m_minimumSize{ 4, 3 };
    int m_resizeBorderThickness = 1;
    int m_titleBarHeight = 1;
};
#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "Core/Rect.h"
#include "UI/Widgets/Widget.h"

class Surface;

namespace Input
{
    class Event;
}

class ContainerWidget : public Widget
{
public:
    using ChildPtr = std::unique_ptr<Widget>;

    ContainerWidget();
    explicit ContainerWidget(const Rect& bounds);
    ~ContainerWidget() override;

    Widget& addChild(ChildPtr child);

    ChildPtr removeChild(const Widget& child);
    ChildPtr removeChildAt(std::size_t index);
    void clearChildren();

    bool containsChild(const Widget& child) const;
    int indexOfChild(const Widget& child) const;

    std::size_t childCount() const;
    bool empty() const;

    Widget* childAt(std::size_t index);
    const Widget* childAt(std::size_t index) const;

    Widget* focusedChild();
    const Widget* focusedChild() const;

    bool setFocusedChild(Widget& child);
    void clearChildFocus();

    bool focusFirstChild();
    bool focusLastChild();
    bool focusNextChild();
    bool focusPreviousChild();

    void update(double deltaTime) override;
    void draw(Surface& surface) override;
    bool handleEvent(const Input::Event& event) override;

protected:
    void onFocusLost() override;

private:
    bool canFocusChild(const Widget& child) const;
    bool focusChildAt(std::size_t index);
    bool focusChildByDirection(int direction);

private:
    std::vector<ChildPtr> m_children;
    Widget* m_focusedChild = nullptr;
};
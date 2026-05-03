#pragma once

#include <string>

#include "Core/Rect.h"
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

private:
    bool m_modal = false;
};
#pragma once

#include <string>
#include <string_view>

#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"
#include "UI/Widgets/Widget.h"

class Surface;

class Label : public Widget
{
public:
    Label();
    explicit Label(std::string text);
    Label(const Rect& bounds, std::string text = {});

    const std::string& text() const;
    void setText(std::string_view text);
    void clearText();
    bool empty() const;

    const Style& textStyle() const;
    void setTextStyle(const Style& style);

    void draw(Surface& surface) override;

private:
    std::string m_text;
    Style m_textStyle;
};
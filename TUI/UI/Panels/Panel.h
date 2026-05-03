#pragma once

#include <string>

#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"
#include "UI/Base/UIElement.h"

class Surface;

class Panel : public UIElement
{
public:
    struct FrameChars
    {
        char32_t topLeft = U'+';
        char32_t topRight = U'+';
        char32_t bottomLeft = U'+';
        char32_t bottomRight = U'+';
        char32_t horizontal = U'-';
        char32_t vertical = U'|';
    };

public:
    Panel();
    explicit Panel(const Rect& bounds);
    Panel(const Rect& bounds, std::string title);

    const std::string& title() const;
    void setTitle(std::string title);
    bool hasTitle() const;

    const Style& backgroundStyle() const;
    void setBackgroundStyle(const Style& style);

    const Style& frameStyle() const;
    void setFrameStyle(const Style& style);

    const Style& titleStyle() const;
    void setTitleStyle(const Style& style);

    const FrameChars& frameChars() const;
    void setFrameChars(const FrameChars& chars);

    Rect contentBounds() const;

    void draw(Surface& surface) override;

private:
    void drawTitle(Surface& surface) const;

private:
    std::string m_title;

    Style m_backgroundStyle;
    Style m_frameStyle;
    Style m_titleStyle;

    FrameChars m_frameChars;
};
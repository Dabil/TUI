#pragma once

#include <string>

#include "Core/Rect.h"
#include "UI/Base/UIElement.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Objects/TextObjectFactory.h"

class Surface;

class Panel : public UIElement
{
public:
    Panel();
    explicit Panel(const Rect& bounds);
    Panel(const Rect& bounds, std::string title);

    const std::string& title() const;
    void setTitle(std::string title);
    bool hasTitle() const;

    const Style& backgroundStyle() const;
    void setBackgroundStyle(const Style& style);

    const Style& borderStyle() const;
    void setBorderStyle(const Style& style);

    const Style& titleStyle() const;
    void setTitleStyle(const Style& style);

    const ObjectFactory::BorderGlyphs& borderGlyphs() const;
    void setBorderGlyphs(const ObjectFactory::BorderGlyphs& glyphs);

    Rect contentBounds() const;

    void draw(Surface& surface) override;

private:
    void drawTitle(Surface& surface) const;

private:
    std::string m_title;

    Style m_backgroundStyle;
    Style m_borderStyle;
    Style m_titleStyle;

    ObjectFactory::BorderGlyphs m_borderGlyphs;
};
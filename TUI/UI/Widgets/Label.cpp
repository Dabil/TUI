#include "UI/Widgets/Label.h"

#include <utility>

#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/UIThemes.h"
#include "Rendering/Surface.h"
#include "Utilities/Text/TextClip.h"

Label::Label()
    : Widget()
    , m_textStyle(UIThemes::Label)
{
    setFocusable(false);
}

Label::Label(std::string text)
    : Widget()
    , m_text(std::move(text))
    , m_textStyle(UIThemes::Label)
{
    setFocusable(false);
}

Label::Label(const Rect& bounds, std::string text)
    : Widget(bounds)
    , m_text(std::move(text))
    , m_textStyle(UIThemes::Label)
{
    setFocusable(false);
}

const std::string& Label::text() const
{
    return m_text;
}

void Label::setText(std::string_view text)
{
    m_text.assign(text.begin(), text.end());
}

void Label::clearText()
{
    m_text.clear();
}

bool Label::empty() const
{
    return m_text.empty();
}

const Style& Label::textStyle() const
{
    return m_textStyle;
}

void Label::setTextStyle(const Style& style)
{
    m_textStyle = style;
}

void Label::draw(Surface& surface)
{
    if (!isVisible())
    {
        return;
    }

    const Rect labelBounds = bounds();

    if (labelBounds.size.width <= 0 || labelBounds.size.height <= 0)
    {
        return;
    }

    if (m_text.empty())
    {
        return;
    }

    const std::string clippedText = TextClip::clipUtf8Text(
        m_text,
        labelBounds.size.width);

    if (clippedText.empty())
    {
        return;
    }

    const Style& renderStyle = isEnabled()
        ? m_textStyle
        : UIThemes::DisabledText;

    surface.buffer().writeString(
        labelBounds.position.x,
        labelBounds.position.y,
        clippedText,
        renderStyle);
}
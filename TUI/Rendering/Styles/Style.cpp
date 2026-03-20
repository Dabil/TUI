#include "Rendering/Styles/Style.h"

Style::Style() = default;

const std::optional<Color>& Style::foreground() const
{
    return m_foreground;
}

const std::optional<Color>& Style::background() const
{
    return m_background;
}

bool Style::bold() const
{
    return m_bold;
}

bool Style::dim() const
{
    return m_dim;
}

bool Style::underline() const
{
    return m_underline;
}

bool Style::slowBlink() const
{
    return m_slowBlink;
}

bool Style::fastBlink() const
{
    return m_fastBlink;
}

bool Style::reverse() const
{
    return m_reverse;
}

bool Style::invisible() const
{
    return m_invisible;
}

bool Style::strike() const
{
    return m_strike;
}

bool Style::hasForeground() const
{
    return m_foreground.has_value();
}

bool Style::hasBackground() const
{
    return m_background.has_value();
}

bool Style::isEmpty() const
{
    return !m_foreground.has_value()
        && !m_background.has_value()
        && !m_bold
        && !m_dim
        && !m_underline
        && !m_slowBlink
        && !m_fastBlink
        && !m_reverse
        && !m_invisible
        && !m_strike;
}

Style Style::withForeground(const Color& color) const
{
    Style copy = *this;
    copy.m_foreground = color;
    return copy;
}

Style Style::withBackground(const Color& color) const
{
    Style copy = *this;
    copy.m_background = color;
    return copy;
}

Style Style::withoutForeground() const
{
    Style copy = *this;
    copy.m_foreground.reset();
    return copy;
}

Style Style::withoutBackground() const
{
    Style copy = *this;
    copy.m_background.reset();
    return copy;
}

Style Style::withBold(bool value) const
{
    Style copy = *this;
    copy.m_bold = value;
    return copy;
}

Style Style::withDim(bool value) const
{
    Style copy = *this;
    copy.m_dim = value;
    return copy;
}

Style Style::withUnderline(bool value) const
{
    Style copy = *this;
    copy.m_underline = value;
    return copy;
}

Style Style::withSlowBlink(bool value) const
{
    Style copy = *this;
    copy.m_slowBlink = value;
    return copy;
}

Style Style::withFastBlink(bool value) const
{
    Style copy = *this;
    copy.m_fastBlink = value;
    return copy;
}

Style Style::withReverse(bool value) const
{
    Style copy = *this;
    copy.m_reverse = value;
    return copy;
}

Style Style::withInvisible(bool value) const
{
    Style copy = *this;
    copy.m_invisible = value;
    return copy;
}

Style Style::withStrike(bool value) const
{
    Style copy = *this;
    copy.m_strike = value;
    return copy;
}

bool Style::operator==(const Style& other) const
{
    return m_foreground == other.m_foreground
        && m_background == other.m_background
        && m_bold == other.m_bold
        && m_dim == other.m_dim
        && m_underline == other.m_underline
        && m_slowBlink == other.m_slowBlink
        && m_fastBlink == other.m_fastBlink
        && m_reverse == other.m_reverse
        && m_invisible == other.m_invisible
        && m_strike == other.m_strike;
}

bool Style::operator!=(const Style& other) const
{
    return !(*this == other);
}
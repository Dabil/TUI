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
    return m_bold.value_or(false);
}

bool Style::dim() const
{
    return m_dim.value_or(false);
}

bool Style::underline() const
{
    return m_underline.value_or(false);
}

bool Style::slowBlink() const
{
    return m_slowBlink.value_or(false);
}

bool Style::fastBlink() const
{
    return m_fastBlink.value_or(false);
}

bool Style::reverse() const
{
    return m_reverse.value_or(false);
}

bool Style::invisible() const
{
    return m_invisible.value_or(false);
}

bool Style::strike() const
{
    return m_strike.value_or(false);
}

const Style::AttributeState& Style::boldState() const
{
    return m_bold;
}

const Style::AttributeState& Style::dimState() const
{
    return m_dim;
}

const Style::AttributeState& Style::underlineState() const
{
    return m_underline;
}

const Style::AttributeState& Style::slowBlinkState() const
{
    return m_slowBlink;
}

const Style::AttributeState& Style::fastBlinkState() const
{
    return m_fastBlink;
}

const Style::AttributeState& Style::reverseState() const
{
    return m_reverse;
}

const Style::AttributeState& Style::invisibleState() const
{
    return m_invisible;
}

const Style::AttributeState& Style::strikeState() const
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

bool Style::hasBold() const
{
    return m_bold.has_value();
}

bool Style::hasDim() const
{
    return m_dim.has_value();
}

bool Style::hasUnderline() const
{
    return m_underline.has_value();
}

bool Style::hasSlowBlink() const
{
    return m_slowBlink.has_value();
}

bool Style::hasFastBlink() const
{
    return m_fastBlink.has_value();
}

bool Style::hasReverse() const
{
    return m_reverse.has_value();
}

bool Style::hasInvisible() const
{
    return m_invisible.has_value();
}

bool Style::hasStrike() const
{
    return m_strike.has_value();
}

bool Style::isEmpty() const
{
    return !m_foreground.has_value()
        && !m_background.has_value()
        && !m_bold.has_value()
        && !m_dim.has_value()
        && !m_underline.has_value()
        && !m_slowBlink.has_value()
        && !m_fastBlink.has_value()
        && !m_reverse.has_value()
        && !m_invisible.has_value()
        && !m_strike.has_value();
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

Style Style::withoutBold() const
{
    Style copy = *this;
    copy.m_bold.reset();
    return copy;
}

Style Style::withoutDim() const
{
    Style copy = *this;
    copy.m_dim.reset();
    return copy;
}

Style Style::withoutUnderline() const
{
    Style copy = *this;
    copy.m_underline.reset();
    return copy;
}

Style Style::withoutSlowBlink() const
{
    Style copy = *this;
    copy.m_slowBlink.reset();
    return copy;
}

Style Style::withoutFastBlink() const
{
    Style copy = *this;
    copy.m_fastBlink.reset();
    return copy;
}

Style Style::withoutReverse() const
{
    Style copy = *this;
    copy.m_reverse.reset();
    return copy;
}

Style Style::withoutInvisible() const
{
    Style copy = *this;
    copy.m_invisible.reset();
    return copy;
}

Style Style::withoutStrike() const
{
    Style copy = *this;
    copy.m_strike.reset();
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
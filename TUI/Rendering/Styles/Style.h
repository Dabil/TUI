#pragma once

#include <optional>

#include "Rendering/Styles/Color.h"

class Style
{
public:
    Style();

    const std::optional<Color>& foreground() const;
    const std::optional<Color>& background() const;

    bool bold() const;
    bool dim() const;
    bool underline() const;
    bool slowBlink() const;
    bool fastBlink() const;
    bool reverse() const;
    bool invisible() const;
    bool strike() const;

    bool hasForeground() const;
    bool hasBackground() const;
    bool isEmpty() const;

    Style withForeground(const Color& color) const;
    Style withBackground(const Color& color) const;

    Style withoutForeground() const;
    Style withoutBackground() const;

    Style withBold(bool value = true) const;
    Style withDim(bool value = true) const;
    Style withUnderline(bool value = true) const;
    Style withSlowBlink(bool value = true) const;
    Style withFastBlink(bool value = true) const;
    Style withReverse(bool value = true) const;
    Style withInvisible(bool value = true) const;
    Style withStrike(bool value = true) const;

    bool operator==(const Style& other) const;
    bool operator!=(const Style& other) const;

private:
    std::optional<Color> m_foreground;
    std::optional<Color> m_background;

    bool m_bold = false;
    bool m_dim = false;
    bool m_underline = false;
    bool m_slowBlink = false;
    bool m_fastBlink = false;
    bool m_reverse = false;
    bool m_invisible = false;
    bool m_strike = false;
};
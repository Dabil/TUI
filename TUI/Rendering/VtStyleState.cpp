#include "Rendering/VtStyleState.h"

#include <string>

#include "Rendering/Styles/Color.h"

namespace
{
    void appendCode(std::string& out, int code)
    {
        if (!out.empty() && out.back() != '[')
        {
            out += ';';
        }

        out += std::to_string(code);
    }

    void appendBasicForeground(std::string& out, Color::Basic color)
    {
        switch (color)
        {
        case Color::Basic::Black:         appendCode(out, 30); break;
        case Color::Basic::Red:           appendCode(out, 31); break;
        case Color::Basic::Green:         appendCode(out, 32); break;
        case Color::Basic::Yellow:        appendCode(out, 33); break;
        case Color::Basic::Blue:          appendCode(out, 34); break;
        case Color::Basic::Magenta:       appendCode(out, 35); break;
        case Color::Basic::Cyan:          appendCode(out, 36); break;
        case Color::Basic::White:         appendCode(out, 37); break;
        case Color::Basic::BrightBlack:   appendCode(out, 90); break;
        case Color::Basic::BrightRed:     appendCode(out, 91); break;
        case Color::Basic::BrightGreen:   appendCode(out, 92); break;
        case Color::Basic::BrightYellow:  appendCode(out, 93); break;
        case Color::Basic::BrightBlue:    appendCode(out, 94); break;
        case Color::Basic::BrightMagenta: appendCode(out, 95); break;
        case Color::Basic::BrightCyan:    appendCode(out, 96); break;
        case Color::Basic::BrightWhite:   appendCode(out, 97); break;
        }
    }

    void appendBasicBackground(std::string& out, Color::Basic color)
    {
        switch (color)
        {
        case Color::Basic::Black:         appendCode(out, 40); break;
        case Color::Basic::Red:           appendCode(out, 41); break;
        case Color::Basic::Green:         appendCode(out, 42); break;
        case Color::Basic::Yellow:        appendCode(out, 43); break;
        case Color::Basic::Blue:          appendCode(out, 44); break;
        case Color::Basic::Magenta:       appendCode(out, 45); break;
        case Color::Basic::Cyan:          appendCode(out, 46); break;
        case Color::Basic::White:         appendCode(out, 47); break;
        case Color::Basic::BrightBlack:   appendCode(out, 100); break;
        case Color::Basic::BrightRed:     appendCode(out, 101); break;
        case Color::Basic::BrightGreen:   appendCode(out, 102); break;
        case Color::Basic::BrightYellow:  appendCode(out, 103); break;
        case Color::Basic::BrightBlue:    appendCode(out, 104); break;
        case Color::Basic::BrightMagenta: appendCode(out, 105); break;
        case Color::Basic::BrightCyan:    appendCode(out, 106); break;
        case Color::Basic::BrightWhite:   appendCode(out, 107); break;
        }
    }

    void appendForeground(std::string& out, const Color& color)
    {
        if (color.isDefault())
        {
            appendCode(out, 39);
            return;
        }

        if (color.isBasic())
        {
            appendBasicForeground(out, color.basic());
            return;
        }

        if (color.isIndexed256())
        {
            appendCode(out, 38);
            appendCode(out, 5);
            appendCode(out, static_cast<int>(color.index256()));
            return;
        }

        if (color.isTrueColor())
        {
            appendCode(out, 38);
            appendCode(out, 2);
            appendCode(out, static_cast<int>(color.red()));
            appendCode(out, static_cast<int>(color.green()));
            appendCode(out, static_cast<int>(color.blue()));
        }
    }

    void appendBackground(std::string& out, const Color& color)
    {
        if (color.isDefault())
        {
            appendCode(out, 49);
            return;
        }

        if (color.isBasic())
        {
            appendBasicBackground(out, color.basic());
            return;
        }

        if (color.isIndexed256())
        {
            appendCode(out, 48);
            appendCode(out, 5);
            appendCode(out, static_cast<int>(color.index256()));
            return;
        }

        if (color.isTrueColor())
        {
            appendCode(out, 48);
            appendCode(out, 2);
            appendCode(out, static_cast<int>(color.red()));
            appendCode(out, static_cast<int>(color.green()));
            appendCode(out, static_cast<int>(color.blue()));
        }
    }
}

void VtStyleState::reset()
{
    m_currentStyle = Style{};
}

const Style& VtStyleState::currentStyle() const
{
    return m_currentStyle;
}

void VtStyleState::appendTransitionTo(std::string& out, const Style& targetStyle)
{
    if (targetStyle == m_currentStyle)
    {
        return;
    }

    if (requiresFullReset(targetStyle))
    {
        appendFullStyle(out, targetStyle);
        m_currentStyle = targetStyle;
        return;
    }

    appendDeltaStyle(out, targetStyle);
    m_currentStyle = targetStyle;
}

void VtStyleState::appendReset(std::string& out)
{
    out += "\x1b[0m";
    m_currentStyle = Style{};
}

bool VtStyleState::requiresFullReset(const Style& targetStyle) const
{
    const bool foregroundRemoved =
        m_currentStyle.hasForeground() && !targetStyle.hasForeground();

    const bool backgroundRemoved =
        m_currentStyle.hasBackground() && !targetStyle.hasBackground();

    const bool boldRemoved =
        m_currentStyle.bold() && !targetStyle.bold();

    const bool dimRemoved =
        m_currentStyle.dim() && !targetStyle.dim();

    const bool underlineRemoved =
        m_currentStyle.underline() && !targetStyle.underline();

    const bool slowBlinkRemoved =
        m_currentStyle.slowBlink() && !targetStyle.slowBlink();

    const bool fastBlinkRemoved =
        m_currentStyle.fastBlink() && !targetStyle.fastBlink();

    const bool reverseRemoved =
        m_currentStyle.reverse() && !targetStyle.reverse();

    const bool invisibleRemoved =
        m_currentStyle.invisible() && !targetStyle.invisible();

    const bool strikeRemoved =
        m_currentStyle.strike() && !targetStyle.strike();

    return foregroundRemoved ||
        backgroundRemoved ||
        boldRemoved ||
        dimRemoved ||
        underlineRemoved ||
        slowBlinkRemoved ||
        fastBlinkRemoved ||
        reverseRemoved ||
        invisibleRemoved ||
        strikeRemoved;
}

void VtStyleState::appendFullStyle(std::string& out, const Style& style) const
{
    std::string sgr = "\x1b[0";

    if (style.hasForeground())
    {
        appendForeground(sgr, *style.foreground());
    }

    if (style.hasBackground())
    {
        appendBackground(sgr, *style.background());
    }

    if (style.bold())      appendCode(sgr, 1);
    if (style.dim())       appendCode(sgr, 2);
    if (style.underline()) appendCode(sgr, 4);
    if (style.slowBlink()) appendCode(sgr, 5);
    if (style.fastBlink()) appendCode(sgr, 6);
    if (style.reverse())   appendCode(sgr, 7);
    if (style.invisible()) appendCode(sgr, 8);
    if (style.strike())    appendCode(sgr, 9);

    sgr += 'm';
    out += sgr;
}

void VtStyleState::appendDeltaStyle(std::string& out, const Style& targetStyle) const
{
    std::string sgr = "\x1b[";

    bool changed = false;

    if (m_currentStyle.foreground() != targetStyle.foreground())
    {
        if (targetStyle.hasForeground())
        {
            appendForeground(sgr, *targetStyle.foreground());
        }
        else
        {
            appendCode(sgr, 39);
        }
        changed = true;
    }

    if (m_currentStyle.background() != targetStyle.background())
    {
        if (targetStyle.hasBackground())
        {
            appendBackground(sgr, *targetStyle.background());
        }
        else
        {
            appendCode(sgr, 49);
        }
        changed = true;
    }

    if (!m_currentStyle.bold() && targetStyle.bold()) { appendCode(sgr, 1); changed = true; }
    if (!m_currentStyle.dim() && targetStyle.dim()) { appendCode(sgr, 2); changed = true; }
    if (!m_currentStyle.underline() && targetStyle.underline()) { appendCode(sgr, 4); changed = true; }
    if (!m_currentStyle.slowBlink() && targetStyle.slowBlink()) { appendCode(sgr, 5); changed = true; }
    if (!m_currentStyle.fastBlink() && targetStyle.fastBlink()) { appendCode(sgr, 6); changed = true; }
    if (!m_currentStyle.reverse() && targetStyle.reverse()) { appendCode(sgr, 7); changed = true; }
    if (!m_currentStyle.invisible() && targetStyle.invisible()) { appendCode(sgr, 8); changed = true; }
    if (!m_currentStyle.strike() && targetStyle.strike()) { appendCode(sgr, 9); changed = true; }

    if (!changed)
    {
        return;
    }

    sgr += 'm';
    out += sgr;
}
#include "UI/Widgets/TextView.h"

#include <algorithm>
#include <cstddef>
#include <utility>

#include "Core/Rect.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Surface.h"
#include "Utilities/Text/TextClip.h"
#include "Utilities/Unicode/UnicodeConversion.h"

TextView::TextView()
    : ScrollablePanel()
    , m_textStyleSet(WidgetStyles::defaultStyleSet(WidgetStyles::Role::TextViewText))
{
}

TextView::TextView(std::string title)
    : ScrollablePanel()
    , m_textStyleSet(WidgetStyles::defaultStyleSet(WidgetStyles::Role::TextViewText))
{
    setTitle(std::move(title));
}

void TextView::setText(std::string_view text)
{
    m_lines = splitLines(text);
    updateContentSizeFromLines();
}

void TextView::setLines(const std::vector<std::string>& lines)
{
    m_lines = lines;
    updateContentSizeFromLines();
}

void TextView::setLines(std::vector<std::string>&& lines)
{
    m_lines = std::move(lines);
    updateContentSizeFromLines();
}

void TextView::appendLine(std::string line)
{
    m_lines.push_back(std::move(line));
    updateContentSizeFromLines();
}

void TextView::clear()
{
    m_lines.clear();
    updateContentSizeFromLines();
}

const std::vector<std::string>& TextView::lines() const
{
    return m_lines;
}

bool TextView::empty() const
{
    return m_lines.empty();
}

std::size_t TextView::lineCount() const
{
    return m_lines.size();
}

void TextView::setTextStyle(const Style& style)
{
    m_textStyleSet.normal = style;
}

const Style& TextView::textStyle() const
{
    return m_textStyleSet.normal;
}

const WidgetStyles::StyleSet& TextView::textStyleSet() const
{
    return m_textStyleSet;
}

void TextView::setTextStyleSet(const WidgetStyles::StyleSet& styleSet)
{
    m_textStyleSet = styleSet;
}

void TextView::drawScrollableContent(
    Surface& surface,
    const Rect& visibleContentRect)
{
    updateContentSizeFromLines();

    ScreenBuffer& buffer = surface.buffer();

    for (int viewY = 0; viewY < visibleContentRect.size.height; ++viewY)
    {
        const int lineIndex = visibleContentRect.position.y + viewY;

        if (lineIndex < 0 || lineIndex >= static_cast<int>(m_lines.size()))
        {
            continue;
        }

        const std::string& sourceLine = m_lines[static_cast<std::size_t>(lineIndex)];
        const int offsetX = visibleContentRect.position.x;

        if (offsetX >= static_cast<int>(sourceLine.size()))
        {
            continue;
        }

        const std::string visibleText = TextClip::clipUtf8Text(
            sourceLine.substr(static_cast<std::size_t>(offsetX)),
            visibleContentRect.size.width);

        buffer.writeString(
            0,
            viewY,
            visibleText,
            WidgetStyles::resolve(
                m_textStyleSet,
                WidgetStyles::stateFor(isEnabled(), false)));
    }
}

void TextView::updateContentSizeFromLines()
{
    int maxWidth = 0;

    for (const std::string& line : m_lines)
    {
        maxWidth = std::max(
            maxWidth,
            TextClip::measureDisplayWidth(UnicodeConversion::utf8ToU32(line)));
    }

    viewport().setContentSize(maxWidth, static_cast<int>(m_lines.size()));
}

std::vector<std::string> TextView::splitLines(std::string_view text)
{
    std::vector<std::string> result;
    std::string current;

    for (char ch : text)
    {
        if (ch == '\r')
        {
            continue;
        }

        if (ch == '\n')
        {
            result.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    result.push_back(current);
    return result;
}
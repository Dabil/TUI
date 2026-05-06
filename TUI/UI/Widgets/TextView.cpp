#include "UI/Widgets/TextView.h"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <utility>
#include < algorithm >

#include "Core/Rect.h"
#include "Core/Size.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Surface.h"
#include "Utilities/Text/TextClip.h"

TextView::TextView(std::string title)
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
    m_textStyle = style;
}

const Style& TextView::textStyle() const
{
    return m_textStyle;
}

void TextView::draw(Surface& surface)
{
    ScrollablePanel::draw(surface);

    ScreenBuffer& buffer = surface.buffer();
    const Rect textRect = resolveTextRect();

    if (textRect.size.width <= 0 || textRect.size.height <= 0)
    {
        return;
    }

    viewport().setViewSize(textRect.size.width, textRect.size.height);
    updateContentSizeFromLines();

    const int offsetX = viewport().scrollX();
    const int offsetY = viewport().scrollY();

    for (int row = 0; row < textRect.size.height; ++row)
    {
        const int lineIndex = offsetY + row;

        if (lineIndex < 0 || lineIndex >= static_cast<int>(m_lines.size()))
        {
            continue;
        }

        const std::string& sourceLine = m_lines[static_cast<std::size_t>(lineIndex)];

        if (offsetX >= static_cast<int>(sourceLine.size()))
        {
            continue;
        }

        const std::string visibleText =
            TextClip::clipUtf8Text(sourceLine.substr(static_cast<std::size_t>(offsetX)), textRect.size.width);

        buffer.writeString(
            textRect.position.x,
            textRect.position.y + row,
            visibleText,
            m_textStyle);
    }
}

void TextView::updateContentSizeFromLines()
{
    int maxWidth = 0;

    for (const std::string& line : m_lines)
    {
        maxWidth = std::max(maxWidth, TextClip::measureDisplayWidth(
            UnicodeConversion::utf8ToU32(line)));
    }

    viewport().setContentSize(maxWidth, static_cast<int>(m_lines.size()));
}

Rect TextView::resolveTextRect() const
{
    Rect rect = bounds();

    // Inner panel area: leave room for border.
    rect.position.x += 1;
    rect.position.y += 1;
    rect.size.width = std::max(0, rect.size.width - 2);
    rect.size.height = std::max(0, rect.size.height - 2);

    return rect;
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
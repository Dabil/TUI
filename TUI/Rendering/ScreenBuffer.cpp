#include "Rendering/ScreenBuffer.h"

#include <stdexcept>

ScreenBuffer::ScreenBuffer() = default;

ScreenBuffer::ScreenBuffer(int width, int height)
{
	resize(width, height);
}

void ScreenBuffer::resize(int width, int height)
{
    if (width < 0 || height < 0)
    {
        throw std::invalid_argument("ScreenBuffer dimensions cannot be negative.");
    }

    m_width = width;
    m_height = height;
    m_cells.assign(static_cast<std::size_t>(m_width * m_height), ScreenCell{});
}

void ScreenBuffer::clear(const Style& style = Style())
{
    for (auto& cell : m_cells)
    {
        cell.glyph = U' ';
        cell.style = style;
    }
}

int ScreenBuffer::getWidth() const
{
    return m_width;
}

int ScreenBuffer::getHeight() const
{
    return m_height;
}

bool ScreenBuffer::inBounds(int x, int y) const
{
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

const ScreenCell& ScreenBuffer::getCell(int x, int y) const
{
    if (!inBounds(x, y))
    {
        throw std::out_of_range("ScreenBuffer::getCell out of bounds.");
    }

    return m_cells[static_cast<std::size_t>(index(x, y))];
}

ScreenCell& ScreenBuffer::getCell(int x, int y)
{
    if (!inBounds(x, y))
    {
        throw std::out_of_range("ScreenBuffer::getCell out of bounds.");
    }

    return m_cells[static_cast<std::size_t>(index(x, y))];
}

void ScreenBuffer::setCell(int x, int y, const ScreenCell& cell)
{
    if (!inBounds(x, y))
    {
        return;
    }

    m_cells[static_cast<std::size_t>(index(x, y))] = cell;
}

void ScreenBuffer::writeChar(int x, int y, char glyph, const Style& style)
{
    if (!inBounds(x, y))
    {
        return;
    }

    ScreenCell& cell = m_cells[static_cast<std::size_t>(index(x, y))];
    cell.glyph = static_cast<char32_t>(static_cast<unsigned char>(glyph));
    cell.style = style;
}

void ScreenBuffer::writeString(int x, int y, const std::string& text, const Style& style)
{
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        writeChar(x + static_cast<int>(i), y, text[i], style);
    }
}

void ScreenBuffer::fillRect(const Rect& rect, char glyph, const Style& style)
{
    const int xStart = std::max(0, rect.position.x);
    const int yStart = std::max(0, rect.position.y);
    const int xEnd = std::min(m_width, rect.position.x + rect.size.width);
    const int yEnd = std::min(m_height, rect.position.y + rect.size.height);

    for (int y = yStart; y < yEnd; ++y)
    {
        for (int x = xStart; x < xEnd; ++x)
        {
            writeChar(x, y, glyph, style);
        }
    }
}

void ScreenBuffer::drawFrame(const Rect& rect, const Style& style)
{
    if (rect.size.width < 2 || rect.size.height < 2)
    {
        return;
    }

    const int left = rect.position.x;
    const int right = rect.position.x + rect.size.width - 1;
    const int top = rect.position.y;
    const int bottom = rect.position.y + rect.size.height - 1;

    writeChar(left, top, '+', style);
    writeChar(right, top, '+', style);
    writeChar(left, bottom, '+', style);
    writeChar(right, bottom, '+', style);

    for (int x = left + 1; x < right; ++x)
    {
        writeChar(x, top, '-', style);
        writeChar(x, bottom, '-', style);
    }

    for (int y = top + 1; y < bottom; ++y)
    {
        writeChar(left, y, '|', style);
        writeChar(right, y, '|', style);
    }
}

std::string ScreenBuffer::renderToString() const
{
    std::string out;
    out.reserve(static_cast<std::size_t>(m_width * m_height + m_height));

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            out.push_back(static_cast<char>(getCell(x, y).glyph));
        }

        if (y + 1 < m_height)
        {
            out.push_back('\n');
        }
    }

    return out;
}

int ScreenBuffer::index(int x, int y) const
{

}
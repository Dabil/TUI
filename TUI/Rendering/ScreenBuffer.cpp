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

}

bool ScreenBuffer::inBounds(int x, int y) const
{

}

const ScreenCell& ScreenBuffer::getCell(int x, int y) const
{

}

ScreenCell& ScreenBuffer::getCell(int x, int y)
{

}

void ScreenBuffer::setCell(int x, int y, const ScreenCell& cell)
{

}

void ScreenBuffer::writeChar(int x, int y, char glyph, const Style& style)
{

}

void ScreenBuffer::writeString(int x, int y, const std::string& text, const Style& style)
{

}

void ScreenBuffer::fillRect(const Rect& rect, char glyph, const Style& style)
{

}

void ScreenBuffer::drawFrame(const Rect& rect, const Style& style)
{

}

std::string ScreenBuffer::renderToString() const
{

}

int ScreenBuffer::index(int x, int y) const
{

}
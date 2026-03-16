#include "Rendering/ScreenBuffer.h"

ScreenBuffer::ScreenBuffer() = default;

ScreenBuffer::ScreenBuffer(int width, int height)
{
	resize(width, height);
}

void ScreenBuffer::resize(int width, int height)
{

}

void ScreenBuffer::clear(const Style& style = Style())
{

}

int ScreenBuffer::getWidth() const
{

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
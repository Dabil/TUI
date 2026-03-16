#include "Rendering/Surface.h"

Surface::Surface() = default;

Surface::Surface(int width, int height)
	  : m_buffer(width, height)
{
}

void Surface::resize(int width, int height)
{
	m_buffer.resize(width, height);
}

void Surface::clear(const Style& style)
{
	m_buffer.clear(style);
}

ScreenBuffer& Surface::buffer()
{

}

const ScreenBuffer& Surface::buffer() const
{

}

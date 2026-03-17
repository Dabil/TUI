#include "ConsoleRenderer.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "Rendering/Styles/Color.h"

ConsoleRenderer::ConsoleRenderer() = default;

ConsoleRenderer::~ConsoleRenderer()
{
    shutdown();
}

bool ConsoleRenderer::initialize() override
{

}

void ConsoleRenderer::shutdown() override
{

}

void ConsoleRenderer::present(const ScreenBuffer& frame) override
{

}

void ConsoleRenderer::resize(int width, int height) override
{

}

int ConsoleRenderer::getConsoleWidth() const
{

}

int ConsoleRenderer::getConsoleHeight() const
{

}

bool ConsoleRenderer::pollResize()
{

}

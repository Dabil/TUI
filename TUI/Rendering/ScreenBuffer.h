#pragma once

#include <string>
#include <vector>

#include "Core/Rect.h"
#include "Rendering/ScreenCell.h"
#include "Rendering/Styles/Style.h"

class ScreenBuffer
{
public:
    ScreenBuffer();
    ScreenBuffer(int width, int height);

    void resize(int width, int height);
    void clear(const Style& style = Style());

    int getWidth() const;
    int getHeight() const;

    bool inBounds(int x, int y) const;

    const ScreenCell& getCell(int x, int y) const;
    ScreenCell& getCell(int x, int y);

    void setCell(int x, int y, const ScreenCell& cell);

    void writeChar(int x, int y, char glyph, const Style& style);
    void writeString(int x, int y, const std::string& text, const Style& style);

    void fillRect(const Rect& rect, char glyph, const Style& style);
    void drawFrame(const Rect& rect, const Style& style);

    std::string renderToString() const;

private:
    int index(int x, int y) const;

private:
    int m_width = 0;
    int m_height = 0;
    std::vector<ScreenCell> m_cells;
};
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Core/Rect.h"
#include "Rendering/ScreenCell.h"
#include "Rendering/Styles/Style.h"

/*
    Purpose:

    ScreenBuffer is the logical cell grid and text placement layer.

    For Phase 1 Unicode readiness:
        - Unicode APIs are the primary public contract
        - narrow string APIs remain as convenience wrappers only
        - width-aware write helpers stay private
        - renderer behavior remains outside this class
*/

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
    const ScreenCell& getCell(int x, int y);

    void setCell(int x, int y, const ScreenCell& cell);
    void setCellStyle(int x, int y, const Style& style);

    // Primary Unicode API
    void writeCodePoint(int x, int y, char32_t glyph, const Style& style);
    void writeText(int x, int y, std::u32string_view text, const Style& style);

    void fillRect(const Rect& rect, char32_t glyph, const Style& style);

    void drawFrame(
        const Rect& rect,
        const Style& style,
        char32_t topLeft = U'+',
        char32_t topRight = U'+',
        char32_t bottomLeft = U'+',
        char32_t bottomRight = U'+',
        char32_t horizontal = U'-',
        char32_t vertical = U'|');

    std::u32string renderToU32String() const;
    std::string renderToUtf8String() const;

    /*
        Single-glyph convenience APIs

        writeChar(char32_t, ...) is the real Unicode single-glyph helper.
        writeUtf8Char(...) is a convenience seam for UTF-8 source text.
        writeChar(char, ...) is legacy compatibility only.
    */
    void writeChar(int x, int y, char32_t glyph, const Style& style);
    void writeUtf8Char(int x, int y, std::string_view utf8Glyph, const Style& style);

    // Legacy convenience wrappers
    void writeChar(int x, int y, char glyph, const Style& style);
    void writeString(int x, int y, const std::string& text, const Style& style);
    std::string renderToString() const;

private:
    int index(int x, int y) const;

    void clearCell(int x, int y);
    void clearOccupiedTrail(int x, int y);
    void writeSingleWidthCodePoint(int x, int y, char32_t glyph, const Style& style);
    void writeDoubleWidthCodePoint(int x, int y, char32_t glyph, const Style& style);

private:
    int m_width = 0;
    int m_height = 0;
    std::vector<ScreenCell> m_cells;
};
#include "Rendering/ScreenBuffer.h"

#include <algorithm>
#include <stdexcept>

#include "Utilities/Unicode/UnicodeConversion.h"
#include "Utilities/Unicode/UnicodeWidth.h"

/*
    This is now the Unicode text placement layer.

    Notes on the narrow compatibility APIs:

    - writeCodePoint(...) and writeText(...) are the real internal text APIs.
    - writeChar(char32_t, ...) is the correct single-glyph Unicode helper.
    - writeUtf8Char(...) decodes UTF-8 and writes the first decoded code point.
    - writeChar(char, ...) remains only for legacy call sites.

    Important:
    A single char is not a real Unicode character API. Therefore the legacy
    writeChar(char, ...) path now routes through UTF-8 decoding of a one-byte
    view instead of promoting the raw byte directly to U+00XX.

    Metadata policy:
    Any operation that clears or overwrites a cell must also reset metadata.
    That keeps ScreenBuffer cell state fully self-consistent and prevents
    stale future-extension flags/priority data from surviving clear paths.
*/

namespace
{
    int cellWidthToInt(CellWidth width)
    {
        switch (width)
        {
        case CellWidth::Zero:
            return 0;

        case CellWidth::Two:
            return 2;

        case CellWidth::One:
        default:
            return 1;
        }
    }

    bool isVisibleLeadingCell(const ScreenCell& cell)
    {
        return cell.kind == CellKind::Glyph;
    }

    char32_t decodeFirstCodePointUtf8(std::string_view utf8Glyph)
    {
        const std::u32string decoded = UnicodeConversion::utf8ToU32(utf8Glyph);

        if (decoded.empty())
        {
            return U' ';
        }

        return decoded.front();
    }
}

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

void ScreenBuffer::clear(const Style& style)
{
    for (auto& cell : m_cells)
    {
        cell.glyph = U' ';
        cell.style = style;
        cell.kind = CellKind::Empty;
        cell.width = CellWidth::One;
        cell.metadata = ScreenCellMetadata{};
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

const ScreenCell& ScreenBuffer::getCell(int x, int y)
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

    // Any direct set must first remove any existing structural occupancy
    // at the target position so we do not leave behind stale wide-trailing
    // or continuation relationships.
    clearOccupiedTrail(x, y);

    ScreenCell normalized = cell;
    normalized.glyph = UnicodeConversion::sanitizeCodePoint(normalized.glyph);

    // Empty cells are normalized to a clean empty state.
    if (normalized.kind == CellKind::Empty)
    {
        ScreenCell& target = m_cells[static_cast<std::size_t>(index(x, y))];
        target.glyph = U' ';
        target.style = normalized.style;
        target.kind = CellKind::Empty;
        target.width = CellWidth::One;
        target.metadata = ScreenCellMetadata{};
        return;
    }

    // A visible glyph cell must obey the measured display width of the glyph.
    if (normalized.kind == CellKind::Glyph)
    {
        const CellWidth measuredWidth =
            UnicodeWidth::measureCodePointWidth(normalized.glyph);

        // Phase 1 policy:
        // Do not materialize a standalone zero-width glyph as an authored
        // visible cell. The buffer is a cell grid, not a full grapheme store.
        if (measuredWidth == CellWidth::Zero)
        {
            ScreenCell& target = m_cells[static_cast<std::size_t>(index(x, y))];
            target.glyph = U' ';
            target.style = normalized.style;
            target.kind = CellKind::Empty;
            target.width = CellWidth::One;
            target.metadata = ScreenCellMetadata{};
            return;
        }

        if (measuredWidth == CellWidth::Two)
        {
            if (!inBounds(x + 1, y))
            {
                return;
            }

            clearOccupiedTrail(x + 1, y);

            ScreenCell& leading = m_cells[static_cast<std::size_t>(index(x, y))];
            leading = normalized;
            leading.kind = CellKind::Glyph;
            leading.width = CellWidth::Two;

            ScreenCell& trailing = m_cells[static_cast<std::size_t>(index(x + 1, y))];
            trailing.glyph = U' ';
            trailing.style = leading.style;
            trailing.kind = CellKind::WideTrailing;
            trailing.width = CellWidth::Zero;
            trailing.metadata = ScreenCellMetadata{};
            return;
        }

        ScreenCell& target = m_cells[static_cast<std::size_t>(index(x, y))];
        target = normalized;
        target.kind = CellKind::Glyph;
        target.width = CellWidth::One;
        return;
    }

    // A trailing cell is only valid when it follows an existing width-2
    // leading glyph cell. setCell() should not create an orphan trailing cell.
    if (normalized.kind == CellKind::WideTrailing)
    {
        if (!inBounds(x - 1, y))
        {
            clearCell(x, y);
            return;
        }

        ScreenCell& left = m_cells[static_cast<std::size_t>(index(x - 1, y))];

        if (left.kind == CellKind::Glyph && left.width == CellWidth::Two)
        {
            ScreenCell& target = m_cells[static_cast<std::size_t>(index(x, y))];
            target.glyph = U' ';
            target.style = left.style;
            target.kind = CellKind::WideTrailing;
            target.width = CellWidth::Zero;
            target.metadata = ScreenCellMetadata{};
            return;
        }

        clearCell(x, y);
        return;
    }

    // Phase 1 does not use authored standalone combining continuation cells
    // as a primary storage model. Normalize them away rather than allowing
    // inconsistent invisible state into the buffer.
    if (normalized.kind == CellKind::CombiningContinuation)
    {
        clearCell(x, y);
        return;
    }

    clearCell(x, y);
}

void ScreenBuffer::setCellStyle(int x, int y, const Style& style)
{
    if (!inBounds(x, y))
    {
        return;
    }

    ScreenCell& cell = m_cells[static_cast<std::size_t>(index(x, y))];

    if (cell.kind == CellKind::Glyph)
    {
        cell.style = style;

        if (cell.width == CellWidth::Two && inBounds(x + 1, y))
        {
            ScreenCell& trailing = m_cells[static_cast<std::size_t>(index(x + 1, y))];

            if (trailing.kind == CellKind::WideTrailing)
            {
                trailing.style = style;
            }
        }

        return;
    }

    if (cell.kind == CellKind::WideTrailing)
    {
        cell.style = style;

        if (inBounds(x - 1, y))
        {
            ScreenCell& leading = m_cells[static_cast<std::size_t>(index(x - 1, y))];

            if (leading.kind == CellKind::Glyph && leading.width == CellWidth::Two)
            {
                leading.style = style;
            }
        }

        return;
    }

    if (cell.kind == CellKind::CombiningContinuation)
    {
        cell.style = style;
        return;
    }

    cell.style = style;
}

void ScreenBuffer::writeCodePoint(int x, int y, char32_t glyph, const Style& style)
{
    if (!inBounds(x, y))
    {
        return;
    }

    glyph = UnicodeConversion::sanitizeCodePoint(glyph);
    const CellWidth width = UnicodeWidth::measureCodePointWidth(glyph);

    if (width == CellWidth::Zero)
    {
        /*
            Phase 1 policy:

            A zero-width code point is treated as attached to the previous
            visible cell on the row if one exists. Because ScreenCell stores
            a single visible glyph only, we keep redraw stable by not
            materializing a separate visible cell for the combining mark.
        */
        for (int previousX = x - 1; previousX >= 0; --previousX)
        {
            const ScreenCell& previousCell =
                m_cells[static_cast<std::size_t>(index(previousX, y))];

            if (previousCell.kind == CellKind::WideTrailing ||
                previousCell.kind == CellKind::CombiningContinuation)
            {
                continue;
            }

            if (isVisibleLeadingCell(previousCell))
            {
                return;
            }

            break;
        }

        return;
    }

    if (width == CellWidth::Two)
    {
        writeDoubleWidthCodePoint(x, y, glyph, style);
        return;
    }

    writeSingleWidthCodePoint(x, y, glyph, style);
}

void ScreenBuffer::writeText(int x, int y, std::u32string_view text, const Style& style)
{
    if (!inBounds(x, y))
    {
        return;
    }

    int cursorX = x;

    for (char32_t glyph : text)
    {
        glyph = UnicodeConversion::sanitizeCodePoint(glyph);

        if (glyph == U'\r')
        {
            continue;
        }

        if (glyph == U'\n')
        {
            break;
        }

        if (cursorX >= m_width)
        {
            break;
        }

        const CellWidth width = UnicodeWidth::measureCodePointWidth(glyph);
        const int advance = cellWidthToInt(width);

        writeCodePoint(cursorX, y, glyph, style);

        if (advance > 0)
        {
            cursorX += advance;
        }
    }
}

void ScreenBuffer::fillRect(const Rect& rect, char32_t glyph, const Style& style)
{
    const int xStart = std::max(0, rect.position.x);
    const int yStart = std::max(0, rect.position.y);
    const int xEnd = std::min(m_width, rect.position.x + rect.size.width);
    const int yEnd = std::min(m_height, rect.position.y + rect.size.height);

    for (int y = yStart; y < yEnd; ++y)
    {
        for (int x = xStart; x < xEnd; ++x)
        {
            writeCodePoint(x, y, glyph, style);
        }
    }
}

void ScreenBuffer::drawFrame(
    const Rect& rect,
    const Style& style,
    char32_t topLeft,
    char32_t topRight,
    char32_t bottomLeft,
    char32_t bottomRight,
    char32_t horizontal,
    char32_t vertical)
{
    if (rect.size.width < 2 || rect.size.height < 2)
    {
        return;
    }

    const int left = rect.position.x;
    const int right = rect.position.x + rect.size.width - 1;
    const int top = rect.position.y;
    const int bottom = rect.position.y + rect.size.height - 1;

    writeCodePoint(left, top, topLeft, style);
    writeCodePoint(right, top, topRight, style);
    writeCodePoint(left, bottom, bottomLeft, style);
    writeCodePoint(right, bottom, bottomRight, style);

    for (int x = left + 1; x < right; ++x)
    {
        writeCodePoint(x, top, horizontal, style);
        writeCodePoint(x, bottom, horizontal, style);
    }

    for (int y = top + 1; y < bottom; ++y)
    {
        writeCodePoint(left, y, vertical, style);
        writeCodePoint(right, y, vertical, style);
    }
}

std::u32string ScreenBuffer::renderToU32String() const
{
    std::u32string out;
    out.reserve(static_cast<std::size_t>(m_width * m_height + m_height));

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            const ScreenCell& cell = getCell(x, y);

            if (cell.kind == CellKind::Glyph)
            {
                out.push_back(cell.glyph);
            }
            else
            {
                out.push_back(U' ');
            }
        }

        if (y + 1 < m_height)
        {
            out.push_back(U'\n');
        }
    }

    return out;
}

std::string ScreenBuffer::renderToUtf8String() const
{
    return UnicodeConversion::u32ToUtf8(renderToU32String());
}

void ScreenBuffer::writeChar(int x, int y, char32_t glyph, const Style& style)
{
    writeCodePoint(x, y, glyph, style);
}

void ScreenBuffer::writeUtf8Char(int x, int y, std::string_view utf8Glyph, const Style& style)
{
    writeCodePoint(x, y, decodeFirstCodePointUtf8(utf8Glyph), style);
}

void ScreenBuffer::writeChar(int x, int y, char glyph, const Style& style)
{
    /*
        Legacy compatibility helper only.

        A single char is not a real Unicode character API. We deliberately
        treat it as a one-byte UTF-8 input fragment and decode through the
        Unicode conversion seam so this function no longer promotes raw bytes
        directly to U+00XX.
    */
    const std::string_view oneByte(&glyph, 1);
    writeUtf8Char(x, y, oneByte, style);
}

void ScreenBuffer::writeString(int x, int y, const std::string& text, const Style& style)
{
    writeText(x, y, UnicodeConversion::utf8ToU32(text), style);
}

std::string ScreenBuffer::renderToString() const
{
    return renderToUtf8String();
}

int ScreenBuffer::index(int x, int y) const
{
    return y * m_width + x;
}

void ScreenBuffer::clearCell(int x, int y)
{
    if (!inBounds(x, y))
    {
        return;
    }

    ScreenCell& cell = m_cells[static_cast<std::size_t>(index(x, y))];
    cell.glyph = U' ';
    cell.style = Style{};
    cell.kind = CellKind::Empty;
    cell.width = CellWidth::One;
    cell.metadata = ScreenCellMetadata{};
}

void ScreenBuffer::clearOccupiedTrail(int x, int y)
{
    if (!inBounds(x, y))
    {
        return;
    }

    ScreenCell& cell = m_cells[static_cast<std::size_t>(index(x, y))];

    if (cell.kind == CellKind::WideTrailing)
    {
        clearCell(x, y);

        if (inBounds(x - 1, y))
        {
            ScreenCell& left = m_cells[static_cast<std::size_t>(index(x - 1, y))];
            if (left.kind == CellKind::Glyph && left.width == CellWidth::Two)
            {
                clearCell(x - 1, y);
            }
        }

        return;
    }

    if (cell.kind == CellKind::Glyph && cell.width == CellWidth::Two)
    {
        clearCell(x, y);

        if (inBounds(x + 1, y))
        {
            ScreenCell& right = m_cells[static_cast<std::size_t>(index(x + 1, y))];
            if (right.kind == CellKind::WideTrailing)
            {
                clearCell(x + 1, y);
            }
        }

        return;
    }

    if (cell.kind == CellKind::CombiningContinuation)
    {
        clearCell(x, y);
        return;
    }

    clearCell(x, y);
}

void ScreenBuffer::writeSingleWidthCodePoint(int x, int y, char32_t glyph, const Style& style)
{
    if (!inBounds(x, y))
    {
        return;
    }

    clearOccupiedTrail(x, y);

    ScreenCell& cell = m_cells[static_cast<std::size_t>(index(x, y))];
    cell.glyph = glyph;
    cell.style = style;
    cell.kind = CellKind::Glyph;
    cell.width = CellWidth::One;
    cell.metadata = ScreenCellMetadata{};
}

void ScreenBuffer::writeDoubleWidthCodePoint(int x, int y, char32_t glyph, const Style& style)
{
    if (!inBounds(x, y))
    {
        return;
    }

    if (!inBounds(x + 1, y))
    {
        return;
    }

    clearOccupiedTrail(x, y);
    clearOccupiedTrail(x + 1, y);

    ScreenCell& leading = m_cells[static_cast<std::size_t>(index(x, y))];
    leading.glyph = glyph;
    leading.style = style;
    leading.kind = CellKind::Glyph;
    leading.width = CellWidth::Two;
    leading.metadata = ScreenCellMetadata{};

    ScreenCell& trailing = m_cells[static_cast<std::size_t>(index(x + 1, y))];
    trailing.glyph = U' ';
    trailing.style = style;
    trailing.kind = CellKind::WideTrailing;
    trailing.width = CellWidth::Zero;
    trailing.metadata = ScreenCellMetadata{};
}
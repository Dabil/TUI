#include "Rendering/Objects/TextObject.h"

#include <stdexcept>

#include "Rendering/Composition/ObjectWriter.h"
#include "Rendering/Objects/PlainTextLoader.h"
#include "Rendering/Objects/TextObjectBlitter.h"
#include "Rendering/Objects/TextObjectExporter.h"
#include "Utilities/Unicode/UnicodeConversion.h"
#include "Utilities/Unicode/UnicodeWidth.h"

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
}

TextObject::TextObject() = default;

TextObject TextObject::fromUtf8(std::string_view utf8Text)
{
    return fromU32(UnicodeConversion::utf8ToU32(utf8Text));
}

TextObject TextObject::fromUtf8(std::string_view utf8Text, const Style& style)
{
    return fromU32(UnicodeConversion::utf8ToU32(utf8Text), style);
}

TextObject TextObject::fromU32(std::u32string_view text)
{
    return buildFromLines(splitLines(text), std::nullopt);
}

TextObject TextObject::fromU32(std::u32string_view text, const Style& style)
{
    return buildFromLines(splitLines(text), std::optional<Style>(style));
}

TextObjectExporter::SaveResult TextObject::saveToFile(const std::string& filePath) const
{
    return TextObjectExporter::saveToFile(*this, filePath);
}

TextObjectExporter::SaveResult TextObject::saveToFile(
    const std::string& filePath,
    const TextObjectExporter::SaveOptions& options) const
{
    return TextObjectExporter::saveToFile(*this, filePath, options);
}

bool TextObject::trySaveToFile(const std::string& filePath) const
{
    return TextObjectExporter::trySaveToFile(*this, filePath);
}

bool TextObject::trySaveToFile(
    const std::string& filePath,
    const TextObjectExporter::SaveOptions& options) const
{
    return TextObjectExporter::trySaveToFile(*this, filePath, options);
}

void TextObject::clear()
{
    m_width = 0;
    m_height = 0;
    m_loaded = false;
    m_hasAnyCellStyles = false;
    m_cells.clear();
}

bool TextObject::isLoaded() const
{
    return m_loaded;
}

int TextObject::getWidth() const
{
    return m_width;
}

int TextObject::getHeight() const
{
    return m_height;
}

bool TextObject::isEmpty() const
{
    return m_cells.empty();
}

bool TextObject::hasAnyCellStyles() const
{
    return m_hasAnyCellStyles;
}

bool TextObject::preservesPerCellStyle() const
{
    return m_hasAnyCellStyles;
}

const TextObjectCell& TextObject::getCell(int x, int y) const
{
    const TextObjectCell* cell = tryGetCell(x, y);
    if (cell == nullptr)
    {
        throw std::out_of_range("TextObject::getCell out of bounds.");
    }

    return *cell;
}

const TextObjectCell* TextObject::tryGetCell(int x, int y) const
{
    if (x < 0 || x >= m_width || y < 0 || y >= m_height)
    {
        return nullptr;
    }

    return &m_cells[static_cast<std::size_t>(index(x, y))];
}

void TextObject::draw(ScreenBuffer& target, int x, int y) const
{
    draw(target, x, y, std::nullopt);
}

void TextObject::draw(ScreenBuffer& target, int x, int y, const Style& overrideStyle) const
{
    draw(target, x, y, std::optional<Style>(overrideStyle));
}

void TextObject::draw(ScreenBuffer& target, int x, int y, const std::optional<Style>& overrideStyle) const
{
    Composition::WritePolicy policy;
    policy.glyphPolicy = Composition::GlyphPolicy::All;
    policy.stylePolicy = Composition::StylePolicy::Apply;
    policy.sourceMask = Composition::SourceMask::GlyphCellsOnly;
    policy.glyphOverwriteRule = Composition::OverwriteRule::Always;
    policy.styleOverwriteRule = Composition::OverwriteRule::Always;
    policy.depthPolicy = Composition::DepthPolicy::Ignore;

    Composition::ObjectWriter writer(target, x, y);
    writer.writeObject(*this, policy, overrideStyle);
}

TextObject TextObject::buildFromLines(
    const std::vector<std::u32string>& lines,
    const std::optional<Style>& defaultStyle)
{
    TextObject object;

    object.m_height = static_cast<int>(lines.size());
    object.m_width = 0;

    for (const std::u32string& line : lines)
    {
        object.m_width = std::max(object.m_width, measureLineWidth(line));
    }

    object.m_cells.reserve(static_cast<std::size_t>(object.m_width * object.m_height));

    for (const std::u32string& line : lines)
    {
        appendLineCells(object.m_cells, line, object.m_width, defaultStyle);
    }

    object.m_hasAnyCellStyles = defaultStyle.has_value();
    object.m_loaded = true;
    return object;
}

std::vector<std::u32string> TextObject::splitLines(std::u32string_view text)
{
    std::vector<std::u32string> lines;
    std::u32string current;

    for (char32_t ch : text)
    {
        if (ch == U'\r')
        {
            continue;
        }

        if (ch == U'\n')
        {
            lines.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    lines.push_back(current);
    return lines;
}

int TextObject::measureLineWidth(std::u32string_view line)
{
    int width = 0;

    for (char32_t glyph : line)
    {
        glyph = UnicodeConversion::sanitizeCodePoint(glyph);

        if (glyph == U'\r' || glyph == U'\n')
        {
            continue;
        }

        const CellWidth measuredWidth = UnicodeWidth::measureCodePointWidth(glyph);
        width += cellWidthToInt(measuredWidth);
    }

    return width;
}

void TextObject::appendLineCells(
    std::vector<TextObjectCell>& outCells,
    std::u32string_view line,
    int paddedWidth,
    const std::optional<Style>& defaultStyle)
{
    int writtenWidth = 0;

    for (char32_t glyph : line)
    {
        glyph = UnicodeConversion::sanitizeCodePoint(glyph);

        if (glyph == U'\r' || glyph == U'\n')
        {
            continue;
        }

        const CellWidth measuredWidth = UnicodeWidth::measureCodePointWidth(glyph);

        if (measuredWidth == CellWidth::Zero)
        {
            continue;
        }

        if (measuredWidth == CellWidth::Two)
        {
            TextObjectCell leading;
            leading.glyph = glyph;
            leading.kind = CellKind::Glyph;
            leading.width = CellWidth::Two;
            leading.style = defaultStyle;
            outCells.push_back(leading);

            TextObjectCell trailing;
            trailing.glyph = U' ';
            trailing.kind = CellKind::WideTrailing;
            trailing.width = CellWidth::Zero;
            trailing.style = defaultStyle;
            outCells.push_back(trailing);

            writtenWidth += 2;
            continue;
        }

        TextObjectCell single;
        single.glyph = glyph;
        single.kind = CellKind::Glyph;
        single.width = CellWidth::One;
        single.style = defaultStyle;
        outCells.push_back(single);

        writtenWidth += 1;
    }

    while (writtenWidth < paddedWidth)
    {
        TextObjectCell empty;
        empty.glyph = U' ';
        empty.kind = CellKind::Empty;
        empty.width = CellWidth::One;
        empty.style.reset();
        outCells.push_back(empty);

        ++writtenWidth;
    }
}

int TextObject::index(int x, int y) const
{
    return y * m_width + x;
}
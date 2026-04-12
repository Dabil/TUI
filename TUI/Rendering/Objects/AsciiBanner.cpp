#include "Rendering/Objects/AsciiBanner.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "Rendering/Objects/TextObjectBuilder.h"
#include "Utilities/Unicode/UnicodeConversion.h"
#include "Utilities/Unicode/UnicodeWidth.h"

namespace
{
    bool startsWith(const std::string& value, const std::string& prefix)
    {
        return value.size() >= prefix.size() &&
            value.compare(0, prefix.size(), prefix) == 0;
    }

    std::u32string decodeUtf8(std::string_view text)
    {
        std::u32string out;
        out.reserve(text.size());

        std::size_t i = 0;
        while (i < text.size())
        {
            const unsigned char c0 = static_cast<unsigned char>(text[i]);

            if (c0 < 0x80)
            {
                out.push_back(static_cast<char32_t>(c0));
                ++i;
                continue;
            }

            if ((c0 & 0xE0) == 0xC0)
            {
                if (i + 1 >= text.size())
                {
                    out.push_back(U'?');
                    break;
                }

                const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
                if ((c1 & 0xC0) != 0x80)
                {
                    out.push_back(U'?');
                    ++i;
                    continue;
                }

                const char32_t cp =
                    ((static_cast<char32_t>(c0 & 0x1F) << 6) |
                        (static_cast<char32_t>(c1 & 0x3F)));

                out.push_back(cp);
                i += 2;
                continue;
            }

            if ((c0 & 0xF0) == 0xE0)
            {
                if (i + 2 >= text.size())
                {
                    out.push_back(U'?');
                    break;
                }

                const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
                const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);

                if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80)
                {
                    out.push_back(U'?');
                    ++i;
                    continue;
                }

                const char32_t cp =
                    ((static_cast<char32_t>(c0 & 0x0F) << 12) |
                        (static_cast<char32_t>(c1 & 0x3F) << 6) |
                        (static_cast<char32_t>(c2 & 0x3F)));

                out.push_back(cp);
                i += 3;
                continue;
            }

            if ((c0 & 0xF8) == 0xF0)
            {
                if (i + 3 >= text.size())
                {
                    out.push_back(U'?');
                    break;
                }

                const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
                const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
                const unsigned char c3 = static_cast<unsigned char>(text[i + 3]);

                if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
                {
                    out.push_back(U'?');
                    ++i;
                    continue;
                }

                const char32_t cp =
                    ((static_cast<char32_t>(c0 & 0x07) << 18) |
                        (static_cast<char32_t>(c1 & 0x3F) << 12) |
                        (static_cast<char32_t>(c2 & 0x3F) << 6) |
                        (static_cast<char32_t>(c3 & 0x3F)));

                out.push_back(cp);
                i += 4;
                continue;
            }

            out.push_back(U'?');
            ++i;
        }

        return out;
    }

    bool parseCodeTag(const std::string& line, int& codepointOut)
    {
        std::istringstream stream(line);
        std::string token;
        if (!(stream >> token))
        {
            return false;
        }

        try
        {
            std::size_t pos = 0;
            long long value = std::stoll(token, &pos, 0);
            if (pos != token.size())
            {
                return false;
            }

            codepointOut = static_cast<int>(value);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool readRawLine(std::ifstream& file, std::string& out)
    {
        out.clear();

        char ch = '\0';
        while (file.get(ch))
        {
            if (ch == '\n')
            {
                break;
            }

            out.push_back(ch);
        }

        if (!out.empty() && out.back() == '\r')
        {
            out.pop_back();
        }

        return !out.empty() || static_cast<bool>(file);
    }

    bool isValidUtf8(std::string_view text)
    {
        std::size_t i = 0;

        while (i < text.size())
        {
            const unsigned char c0 = static_cast<unsigned char>(text[i]);

            if (c0 < 0x80)
            {
                ++i;
                continue;
            }

            if ((c0 & 0xE0) == 0xC0)
            {
                if (i + 1 >= text.size())
                {
                    return false;
                }

                const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
                if ((c1 & 0xC0) != 0x80)
                {
                    return false;
                }

                i += 2;
                continue;
            }

            if ((c0 & 0xF0) == 0xE0)
            {
                if (i + 2 >= text.size())
                {
                    return false;
                }

                const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
                const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);

                if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80)
                {
                    return false;
                }

                i += 3;
                continue;
            }

            if ((c0 & 0xF8) == 0xF0)
            {
                if (i + 3 >= text.size())
                {
                    return false;
                }

                const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
                const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
                const unsigned char c3 = static_cast<unsigned char>(text[i + 3]);

                if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
                {
                    return false;
                }

                i += 4;
                continue;
            }

            return false;
        }

        return true;
    }

    void appendUtf8(std::string& out, char32_t cp)
    {
        if (cp <= 0x7F)
        {
            out.push_back(static_cast<char>(cp));
        }
        else if (cp <= 0x7FF)
        {
            out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp <= 0xFFFF)
        {
            out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
        {
            out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    char32_t cp437ToCodePoint(unsigned char b)
    {
        static const char32_t table[256] =
        {
            0x0000,0x263A,0x263B,0x2665,0x2666,0x2663,0x2660,0x2022,
            0x25D8,0x25CB,0x25D9,0x2642,0x2640,0x266A,0x266B,0x263C,
            0x25BA,0x25C4,0x2195,0x203C,0x00B6,0x00A7,0x25AC,0x21A8,
            0x2191,0x2193,0x2192,0x2190,0x221F,0x2194,0x25B2,0x25BC,
            0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
            0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
            0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
            0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
            0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
            0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
            0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
            0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
            0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
            0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
            0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
            0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x2302,
            0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,
            0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,
            0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,
            0x00FF,0x00D6,0x00DC,0x00A2,0x00A3,0x00A5,0x20A7,0x0192,
            0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,
            0x00BF,0x2310,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
            0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
            0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
            0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
            0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
            0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
            0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
            0x03B1,0x00DF,0x0393,0x03C0,0x03A3,0x03C3,0x00B5,0x03C4,
            0x03A6,0x0398,0x03A9,0x03B4,0x221E,0x03C6,0x03B5,0x2229,
            0x2261,0x00B1,0x2265,0x2264,0x2320,0x2321,0x00F7,0x2248,
            0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0
        };

        return table[b];
    }

    std::string decodeFontLineBytes(std::string_view raw)
    {
        if (isValidUtf8(raw))
        {
            return std::string(raw);
        }

        std::string utf8;
        utf8.reserve(raw.size() * 3);

        for (unsigned char b : raw)
        {
            appendUtf8(utf8, cp437ToCodePoint(b));
        }

        return utf8;
    }

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

void AsciiBanner::setFigletFontDirectory(const std::filesystem::path& path)
{
    m_figletFontDirectory = path;
}

void AsciiBanner::setToiletFontDirectory(const std::filesystem::path& path)
{
    m_toiletFontDirectory = path;
}

const std::filesystem::path& AsciiBanner::getFigletFontDirectory() const
{
    return m_figletFontDirectory;
}

const std::filesystem::path& AsciiBanner::getToiletFontDirectory() const
{
    return m_toiletFontDirectory;
}

bool AsciiBanner::hasFontLoaded() const
{
    return m_hasLoadedFont;
}

std::string AsciiBanner::getLoadedFontName() const
{
    return m_font.name;
}

std::filesystem::path AsciiBanner::getLoadedFontPath() const
{
    return m_font.path;
}

AsciiBanner::FontKind AsciiBanner::getLoadedFontKind() const
{
    return m_font.kind;
}

int AsciiBanner::getFontHeight() const
{
    return m_font.height;
}

std::string AsciiBanner::trimRight(const std::string& s)
{
    std::string result = s;

    while (!result.empty() &&
        (result.back() == ' ' || result.back() == '\t' || result.back() == '\r'))
    {
        result.pop_back();
    }

    return result;
}

std::string AsciiBanner::trimEndMarker(const std::string& s, char endMarker)
{
    std::string result = s;

    while (!result.empty() && (result.back() == endMarker || result.back() == '\r'))
    {
        result.pop_back();
    }

    return result;
}

std::vector<std::string> AsciiBanner::splitLines(std::string_view text)
{
    std::vector<std::string> lines;

    std::stringstream ss{ std::string(text) };
    std::string line;

    while (std::getline(ss, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        lines.push_back(line);
    }

    if (lines.empty())
    {
        lines.push_back("");
    }

    return lines;
}

std::filesystem::path AsciiBanner::buildFontPath(
    FontKind kind,
    const std::filesystem::path& figletDir,
    const std::filesystem::path& toiletDir,
    const std::string& fontNameWithoutExtension)
{
    const auto& base = (kind == FontKind::FIGlet) ? figletDir : toiletDir;
    const char* extension = (kind == FontKind::FIGlet) ? ".flf" : ".tlf";
    return base / (fontNameWithoutExtension + extension);
}

bool AsciiBanner::loadFont(FontKind kind, const std::string& fontNameWithoutExtension)
{
    return loadFontFile(
        buildFontPath(kind, m_figletFontDirectory, m_toiletFontDirectory, fontNameWithoutExtension));
}

bool AsciiBanner::loadFontFile(const std::filesystem::path& fontFile)
{
    std::ifstream file(fontFile, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    FontData newFont;
    newFont.path = fontFile;
    newFont.name = fontFile.stem().string();
    newFont.kind = (fontFile.extension() == ".tlf") ? FontKind::Toilet : FontKind::FIGlet;

    std::string header;
    if (!readRawLine(file, header))
    {
        return false;
    }

    header = trimRight(header);

    const bool isFigletHeader = startsWith(header, "flf2a");
    const bool isToiletHeader = startsWith(header, "tlf2a");

    if ((!isFigletHeader && !isToiletHeader) || header.size() < 6)
    {
        return false;
    }

    newFont.hardBlank = header[5];

    std::istringstream headerStream(header.substr(6));
    headerStream
        >> newFont.height
        >> newFont.baseline
        >> newFont.maxLength
        >> newFont.oldLayout
        >> newFont.commentLines
        >> newFont.printDirection
        >> newFont.fullLayout
        >> newFont.codetagCount;

    if (newFont.height <= 0)
    {
        return false;
    }

    for (int i = 0; i < newFont.commentLines; ++i)
    {
        std::string ignored;
        if (!readRawLine(file, ignored))
        {
            return false;
        }
    }

    for (int codepoint = 32; codepoint <= 126; ++codepoint)
    {
        std::vector<std::string> glyph;
        glyph.reserve(static_cast<std::size_t>(newFont.height));

        char endMarker = '\0';

        for (int row = 0; row < newFont.height; ++row)
        {
            std::string rawLine;
            if (!readRawLine(file, rawLine))
            {
                return false;
            }

            if (row == 0)
            {
                endMarker = rawLine.empty() ? '@' : rawLine.back();
            }

            const std::string decoded = decodeFontLineBytes(rawLine);
            glyph.push_back(trimEndMarker(decoded, endMarker));
        }

        newFont.glyphs[codepoint] = std::move(glyph);
    }

    while (file)
    {
        const std::streampos blockStart = file.tellg();
        std::string codeTagLine;

        if (!readRawLine(file, codeTagLine))
        {
            break;
        }

        codeTagLine = trimRight(codeTagLine);
        if (codeTagLine.empty())
        {
            continue;
        }

        int codepoint = 0;
        if (!parseCodeTag(codeTagLine, codepoint))
        {
            continue;
        }

        std::vector<std::string> glyph;
        glyph.reserve(static_cast<std::size_t>(newFont.height));

        char endMarker = '\0';

        for (int row = 0; row < newFont.height; ++row)
        {
            std::string rawLine;
            if (!readRawLine(file, rawLine))
            {
                file.clear();
                file.seekg(blockStart);
                break;
            }

            if (row == 0)
            {
                endMarker = rawLine.empty() ? '@' : rawLine.back();
            }

            const std::string decoded = decodeFontLineBytes(rawLine);
            glyph.push_back(trimEndMarker(decoded, endMarker));
        }

        if (static_cast<int>(glyph.size()) == newFont.height)
        {
            newFont.glyphs[codepoint] = std::move(glyph);
        }
        else
        {
            break;
        }
    }

    m_font = std::move(newFont);
    m_hasLoadedFont = true;
    return true;
}

const std::vector<std::string>* AsciiBanner::findGlyph(int codepoint) const
{
    auto it = m_font.glyphs.find(codepoint);
    if (it != m_font.glyphs.end())
    {
        return &it->second;
    }

    return nullptr;
}

const std::vector<std::string>& AsciiBanner::getFallbackGlyph() const
{
    if (const auto* questionGlyph = findGlyph('?'))
    {
        return *questionGlyph;
    }

    static const std::vector<std::string> empty;
    return empty;
}

int AsciiBanner::getMinimumLeadingSpaces(const std::vector<std::string>& glyph)
{
    int minSpaces = std::numeric_limits<int>::max();

    for (const auto& line : glyph)
    {
        const std::u32string decoded = decodeUtf8(line);

        int count = 0;
        while (count < static_cast<int>(decoded.size()) &&
            decoded[static_cast<std::size_t>(count)] == U' ')
        {
            ++count;
        }

        if (count < static_cast<int>(decoded.size()))
        {
            minSpaces = (std::min)(minSpaces, count);
        }
    }

    return (minSpaces == std::numeric_limits<int>::max()) ? 0 : minSpaces;
}

int AsciiBanner::getMinimumTrailingSpaces(const std::vector<std::string>& lines)
{
    int minSpaces = std::numeric_limits<int>::max();

    for (const auto& line : lines)
    {
        const std::u32string decoded = decodeUtf8(line);

        int count = 0;
        for (int i = static_cast<int>(decoded.size()) - 1;
            i >= 0 && decoded[static_cast<std::size_t>(i)] == U' ';
            --i)
        {
            ++count;
        }

        if (!decoded.empty())
        {
            minSpaces = (std::min)(minSpaces, count);
        }
    }

    return (minSpaces == std::numeric_limits<int>::max()) ? 0 : minSpaces;
}

void AsciiBanner::appendGlyphFullWidth(
    std::vector<std::string>& outputLines,
    const std::vector<std::string>& glyph)
{
    for (std::size_t row = 0; row < outputLines.size(); ++row)
    {
        outputLines[row] += glyph[row];
    }
}

void AsciiBanner::appendGlyphKern(
    std::vector<std::string>& outputLines,
    const std::vector<std::string>& glyph)
{
    bool allEmpty = true;

    for (const auto& line : outputLines)
    {
        if (!line.empty())
        {
            allEmpty = false;
            break;
        }
    }

    if (allEmpty)
    {
        appendGlyphFullWidth(outputLines, glyph);
        return;
    }

    const int trailing = getMinimumTrailingSpaces(outputLines);
    const int leading = getMinimumLeadingSpaces(glyph);
    const int overlap = (std::min)(trailing, leading);

    for (std::size_t row = 0; row < outputLines.size(); ++row)
    {
        int removed = 0;

        while (removed < overlap)
        {
            const std::u32string decoded = decodeUtf8(outputLines[row]);
            if (decoded.empty() || decoded.back() != U' ')
            {
                break;
            }

            std::string trimmed = outputLines[row];
            while (!trimmed.empty())
            {
                const unsigned char c = static_cast<unsigned char>(trimmed.back());
                trimmed.pop_back();

                if ((c & 0xC0) != 0x80)
                {
                    break;
                }
            }

            outputLines[row] = trimmed;
            ++removed;
        }

        outputLines[row] += glyph[row];
    }
}

std::vector<std::string> AsciiBanner::renderSingleLine(
    std::string_view text,
    const RenderOptions& options) const
{
    if (!m_hasLoadedFont)
    {
        throw std::runtime_error("AsciiBanner::renderSingleLine(): no font loaded.");
    }

    std::vector<std::string> outputLines(static_cast<std::size_t>(m_font.height), "");

    const std::u32string codepoints = decodeUtf8(text);

    for (char32_t cp : codepoints)
    {
        const std::vector<std::string>* glyph = findGlyph(static_cast<int>(cp));

        if (!glyph)
        {
            glyph = options.useFallbackGlyphForUnknownChars ? &getFallbackGlyph() : nullptr;
        }

        if (!glyph || glyph->empty())
        {
            continue;
        }

        std::vector<std::string> processedGlyph = *glyph;

        if (options.replaceHardBlankWithSpace)
        {
            for (auto& row : processedGlyph)
            {
                std::replace(row.begin(), row.end(), m_font.hardBlank, ' ');
            }
        }

        if (options.composeMode == ComposeMode::FullWidth)
        {
            appendGlyphFullWidth(outputLines, processedGlyph);
        }
        else
        {
            appendGlyphKern(outputLines, processedGlyph);
        }
    }

    return outputLines;
}

std::vector<std::string> AsciiBanner::alignLines(
    const std::vector<std::string>& lines,
    Alignment alignment,
    std::size_t targetWidth)
{
    if (alignment == Alignment::Left || targetWidth == 0)
    {
        return lines;
    }

    std::vector<std::string> out = lines;

    for (auto& line : out)
    {
        const std::u32string decoded = UnicodeConversion::utf8ToU32(line);

        std::size_t width = 0;
        for (char32_t cp : decoded)
        {
            width += static_cast<std::size_t>(
                cellWidthToInt(UnicodeWidth::measureCodePointWidth(
                    UnicodeConversion::sanitizeCodePoint(cp))));
        }

        if (width >= targetWidth)
        {
            continue;
        }

        const std::size_t pad = targetWidth - width;
        std::size_t leftPad = 0;

        if (alignment == Alignment::Center)
        {
            leftPad = pad / 2;
        }
        else if (alignment == Alignment::Right)
        {
            leftPad = pad;
        }

        line = std::string(leftPad, ' ') + line;
    }

    return out;
}

std::string AsciiBanner::joinLines(
    const std::vector<std::string>& lines,
    bool trimTrailingSpaces)
{
    std::ostringstream out;

    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        out << (trimTrailingSpaces ? trimRight(lines[i]) : lines[i]);

        if (i + 1 < lines.size())
        {
            out << '\n';
        }
    }

    return out.str();
}

std::string AsciiBanner::render(std::string_view text) const
{
    RenderOptions options;
    return render(text, options);
}

std::string AsciiBanner::render(std::string_view text, const RenderOptions& options) const
{
    const std::vector<std::string> lines = renderLines(text, options);
    return joinLines(lines, false);
}

std::vector<std::string> AsciiBanner::renderLines(std::string_view text) const
{
    RenderOptions options;
    return renderLines(text, options);
}

std::vector<std::string> AsciiBanner::renderLines(
    std::string_view text,
    const RenderOptions& options) const
{
    if (!m_hasLoadedFont)
    {
        throw std::runtime_error("AsciiBanner::renderLines(): no font loaded.");
    }

    std::vector<std::string> result;
    const std::vector<std::string> inputLines = splitLines(text);

    for (std::size_t i = 0; i < inputLines.size(); ++i)
    {
        std::vector<std::string> rendered = renderSingleLine(inputLines[i], options);
        rendered = alignLines(rendered, options.alignment, options.targetWidth);

        if (options.trimTrailingSpaces)
        {
            for (auto& line : rendered)
            {
                line = trimRight(line);
            }
        }

        result.insert(result.end(), rendered.begin(), rendered.end());

        if (i + 1 < inputLines.size())
        {
            result.emplace_back("");
        }
    }

    return result;
}

TextObject AsciiBanner::generateTextObject(std::string_view text) const
{
    RenderOptions options;
    return generateTextObject(text, options);
}

TextObject AsciiBanner::generateTextObject(
    std::string_view text,
    const RenderOptions& options) const
{
    return generateTextObject(text, Style{}, options);
}

TextObject AsciiBanner::generateTextObject(
    std::string_view text,
    const Style& style) const
{
    RenderOptions options;
    return generateTextObject(text, style, options);
}

TextObject AsciiBanner::generateTextObject(
    std::string_view text,
    const Style& style,
    const RenderOptions& options) const
{
    const std::vector<std::string> lines = renderLines(text, options);

    int width = 0;
    for (const std::string& line : lines)
    {
        width = (std::max)(width, measureRenderedLineWidth(line));
    }

    const int height = static_cast<int>(lines.size());

    if (width <= 0 || height <= 0)
    {
        return TextObject{};
    }

    TextObjectBuilder builder(width, height);

    for (int y = 0; y < height; ++y)
    {
        appendRenderedLineToBuilder(
            builder,
            y,
            lines[static_cast<std::size_t>(y)],
            options.transparentSpaces,
            std::optional<Style>(style));
    }

    return builder.build();
}

int AsciiBanner::measureRenderedLineWidth(std::string_view line)
{
    const std::u32string decoded = UnicodeConversion::utf8ToU32(line);

    int width = 0;
    for (char32_t cp : decoded)
    {
        cp = UnicodeConversion::sanitizeCodePoint(cp);
        width += cellWidthToInt(UnicodeWidth::measureCodePointWidth(cp));
    }

    return width;
}

void AsciiBanner::appendRenderedLineToBuilder(
    TextObjectBuilder& builder,
    int y,
    std::string_view line,
    bool transparentSpaces,
    const std::optional<Style>& style)
{
    const std::u32string decoded = UnicodeConversion::utf8ToU32(line);

    int x = 0;
    for (char32_t cp : decoded)
    {
        cp = UnicodeConversion::sanitizeCodePoint(cp);

        if (cp == U'\r' || cp == U'\n')
        {
            continue;
        }

        const CellWidth measuredWidth = UnicodeWidth::measureCodePointWidth(cp);

        if (measuredWidth == CellWidth::Zero)
        {
            continue;
        }

        if (cp == U' ' && transparentSpaces)
        {
            if (measuredWidth == CellWidth::Two)
            {
                builder.setEmpty(x, y);
                builder.setEmpty(x + 1, y);
                x += 2;
            }
            else
            {
                builder.setEmpty(x, y);
                x += 1;
            }

            continue;
        }

        if (measuredWidth == CellWidth::Two)
        {
            builder.setWideGlyph(x, y, cp, style);
            x += 2;
            continue;
        }

        builder.setGlyph(x, y, cp, style);
        x += 1;
    }
}
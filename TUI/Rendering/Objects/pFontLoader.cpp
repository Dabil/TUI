#include "Rendering/Objects/pFontLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

#include "Rendering/Objects/TextObjectBuilder.h"
#include "Utilities/Unicode/UnicodeConversion.h"

/*
    Keep this pseudo-font path parallel to AsciiBannerFont rather than 
    trying to force it into the FIGlet model. 

    AsciiBannerFont is fundamentally row-based banner art from FLF/TLF 
    sources, while pseudo-graphical authored fonts want richer per-cell 
    semantics and simpler deterministic mapping assets. 

    Parallel models with a shared TextObject output are the cleaner 
    long-term design.
*/

namespace
{
    using namespace PseudoFont;

    std::string trim(const std::string& input)
    {
        std::size_t begin = 0;
        while (begin < input.size() && std::isspace(static_cast<unsigned char>(input[begin])))
        {
            ++begin;
        }

        std::size_t end = input.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])))
        {
            --end;
        }

        return input.substr(begin, end - begin);
    }

    bool startsWith(const std::string& value, const std::string& prefix)
    {
        return value.size() >= prefix.size() &&
            value.compare(0, prefix.size(), prefix) == 0;
    }

    bool iequals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < a.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i])))
            {
                return false;
            }
        }

        return true;
    }

    bool parseBool(const std::string& value, bool& outValue)
    {
        if (iequals(value, "true") || iequals(value, "1") || iequals(value, "yes"))
        {
            outValue = true;
            return true;
        }

        if (iequals(value, "false") || iequals(value, "0") || iequals(value, "no"))
        {
            outValue = false;
            return true;
        }

        return false;
    }

    bool parseInt(const std::string& value, int& outValue)
    {
        try
        {
            std::size_t pos = 0;
            const int parsed = std::stoi(value, &pos, 10);
            if (pos != value.size())
            {
                return false;
            }

            outValue = parsed;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    std::optional<Color> parseBasicColor(const std::string& value)
    {
        const struct Entry
        {
            const char* name;
            Color::Basic basic;
        } table[] =
        {
            { "black", Color::Basic::Black },
            { "red", Color::Basic::Red },
            { "green", Color::Basic::Green },
            { "yellow", Color::Basic::Yellow },
            { "blue", Color::Basic::Blue },
            { "magenta", Color::Basic::Magenta },
            { "cyan", Color::Basic::Cyan },
            { "white", Color::Basic::White },
            { "bright_black", Color::Basic::BrightBlack },
            { "bright_red", Color::Basic::BrightRed },
            { "bright_green", Color::Basic::BrightGreen },
            { "bright_yellow", Color::Basic::BrightYellow },
            { "bright_blue", Color::Basic::BrightBlue },
            { "bright_magenta", Color::Basic::BrightMagenta },
            { "bright_cyan", Color::Basic::BrightCyan },
            { "bright_white", Color::Basic::BrightWhite }
        };

        for (const Entry& entry : table)
        {
            if (iequals(value, entry.name))
            {
                return Color::FromBasic(entry.basic);
            }
        }

        return std::nullopt;
    }

    bool hexNibble(const char ch, std::uint8_t& out)
    {
        if (ch >= '0' && ch <= '9')
        {
            out = static_cast<std::uint8_t>(ch - '0');
            return true;
        }

        if (ch >= 'a' && ch <= 'f')
        {
            out = static_cast<std::uint8_t>(10 + (ch - 'a'));
            return true;
        }

        if (ch >= 'A' && ch <= 'F')
        {
            out = static_cast<std::uint8_t>(10 + (ch - 'A'));
            return true;
        }

        return false;
    }

    std::optional<Color> parseRgbColor(const std::string& value)
    {
        if (value.size() != 7 || value[0] != '#')
        {
            return std::nullopt;
        }

        std::uint8_t n0 = 0;
        std::uint8_t n1 = 0;
        std::uint8_t n2 = 0;
        std::uint8_t n3 = 0;
        std::uint8_t n4 = 0;
        std::uint8_t n5 = 0;

        if (!hexNibble(value[1], n0) || !hexNibble(value[2], n1) ||
            !hexNibble(value[3], n2) || !hexNibble(value[4], n3) ||
            !hexNibble(value[5], n4) || !hexNibble(value[6], n5))
        {
            return std::nullopt;
        }

        const std::uint8_t r = static_cast<std::uint8_t>((n0 << 4) | n1);
        const std::uint8_t g = static_cast<std::uint8_t>((n2 << 4) | n3);
        const std::uint8_t b = static_cast<std::uint8_t>((n4 << 4) | n5);

        return Color::FromRgb(r, g, b);
    }

    bool applyStyleProperty(
        Style& style,
        const std::string& key,
        const std::string& value)
    {
        if (iequals(key, "foreground"))
        {
            if (iequals(value, "none") || iequals(value, "default"))
            {
                style = style.withoutForeground();
                return true;
            }

            if (const std::optional<Color> basic = parseBasicColor(value))
            {
                style = style.withForeground(*basic);
                return true;
            }

            if (const std::optional<Color> rgb = parseRgbColor(value))
            {
                style = style.withForeground(*rgb);
                return true;
            }

            return false;
        }

        if (iequals(key, "background"))
        {
            if (iequals(value, "none") || iequals(value, "default"))
            {
                style = style.withoutBackground();
                return true;
            }

            if (const std::optional<Color> basic = parseBasicColor(value))
            {
                style = style.withBackground(*basic);
                return true;
            }

            if (const std::optional<Color> rgb = parseRgbColor(value))
            {
                style = style.withBackground(*rgb);
                return true;
            }

            return false;
        }

        bool boolValue = false;
        if (!parseBool(value, boolValue))
        {
            return false;
        }

        if (iequals(key, "bold"))
        {
            style = style.withBold(boolValue);
            return true;
        }

        if (iequals(key, "dim"))
        {
            style = style.withDim(boolValue);
            return true;
        }

        if (iequals(key, "underline"))
        {
            style = style.withUnderline(boolValue);
            return true;
        }

        if (iequals(key, "reverse"))
        {
            style = style.withReverse(boolValue);
            return true;
        }

        if (iequals(key, "strike"))
        {
            style = style.withStrike(boolValue);
            return true;
        }

        if (iequals(key, "slowblink"))
        {
            style = style.withSlowBlink(boolValue);
            return true;
        }

        if (iequals(key, "fastblink"))
        {
            style = style.withFastBlink(boolValue);
            return true;
        }

        if (iequals(key, "invisible"))
        {
            style = style.withInvisible(boolValue);
            return true;
        }

        return false;
    }

    bool parseCodePointToken(const std::string& token, char32_t& outCodePoint)
    {
        if (iequals(token, "space"))
        {
            outCodePoint = U' ';
            return true;
        }

        if (iequals(token, "tab"))
        {
            outCodePoint = U'\t';
            return true;
        }

        if (iequals(token, "question"))
        {
            outCodePoint = U'?';
            return true;
        }

        if (startsWith(token, "U+") || startsWith(token, "u+"))
        {
            try
            {
                const unsigned long value = std::stoul(token.substr(2), nullptr, 16);
                outCodePoint = static_cast<char32_t>(value);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        const std::u32string decoded = UnicodeConversion::utf8ToU32(token);
        if (decoded.size() != 1)
        {
            return false;
        }

        outCodePoint = decoded.front();
        return true;
    }

    void addWarning(
        LoadResult& result,
        const LoadWarningCode code,
        const std::string& message)
    {
        LoadWarning warning;
        warning.code = code;
        warning.message = message;
        result.warnings.push_back(std::move(warning));
    }

    std::vector<std::string> splitLogicalLines(std::string_view text)
    {
        std::vector<std::string> lines;
        std::string current;

        for (char ch : text)
        {
            if (ch == '\r')
            {
                continue;
            }

            if (ch == '\n')
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

    std::vector<int> computeOpaqueColumnBounds(
        const GlyphPattern& glyph,
        const bool trimTransparentColumns)
    {
        if (!trimTransparentColumns || !glyph.isValid())
        {
            return { 0, glyph.width };
        }

        int minX = glyph.width;
        int maxX = -1;

        for (int y = 0; y < glyph.height; ++y)
        {
            for (int x = 0; x < glyph.width; ++x)
            {
                const GlyphCell& cell = glyph.getCell(x, y);
                if (!cell.transparent)
                {
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                }
            }
        }

        if (maxX < minX)
        {
            return { 0, glyph.width };
        }

        return { minX, maxX + 1 };
    }

    int measureLineWidth(
        const FontDefinition& font,
        const std::u32string& line,
        const RenderOptions& options)
    {
        int width = 0;
        bool firstGlyph = true;

        for (char32_t cp : line)
        {
            if (cp == U'\t')
            {
                cp = U' ';
            }

            const GlyphPattern* glyph = font.findGlyph(cp);
            if (glyph == nullptr && options.useFallbackGlyphForUnknownChars)
            {
                glyph = font.findFallbackGlyph();
            }

            if (glyph == nullptr)
            {
                continue;
            }

            if (!firstGlyph)
            {
                width += (cp == U' ') ? font.spacing.wordSpacing : font.spacing.letterSpacing;
            }

            const std::vector<int> bounds = computeOpaqueColumnBounds(
                *glyph,
                options.trimTrailingTransparentColumns);

            width += std::max(0, bounds[1] - bounds[0]);
            firstGlyph = false;
        }

        return width;
    }

    std::optional<Style> chooseCellStyle(
        const GlyphCell& cell,
        const GlyphPattern& glyph,
        const FontDefinition& font,
        const std::optional<Style>& overrideStyle,
        const RenderOptions& options)
    {
        if (overrideStyle.has_value())
        {
            return overrideStyle;
        }

        if (options.preserveGlyphStyles)
        {
            if (cell.style.has_value())
            {
                return cell.style;
            }

            if (glyph.defaultStyle.has_value())
            {
                return glyph.defaultStyle;
            }

            if (font.defaultStyle.has_value())
            {
                return font.defaultStyle;
            }
        }

        return std::nullopt;
    }

    TextObject buildTextObject(
        const FontDefinition& font,
        const std::vector<std::u32string>& lines,
        const std::optional<Style>& overrideStyle,
        const RenderOptions& options)
    {
        if (!font.isLoaded())
        {
            return {};
        }

        int contentHeight = 0;
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            contentHeight += font.glyphHeight;
            if (i + 1 < lines.size())
            {
                contentHeight += font.spacing.lineSpacing;
            }
        }

        int measuredWidth = 0;
        for (const std::u32string& line : lines)
        {
            measuredWidth = std::max(measuredWidth, measureLineWidth(font, line, options));
        }

        int outputWidth = measuredWidth;
        if (options.targetWidth > 0)
        {
            outputWidth = std::max(outputWidth, static_cast<int>(options.targetWidth));
        }

        if (outputWidth <= 0 || contentHeight <= 0)
        {
            return {};
        }

        TextObjectBuilder builder(outputWidth, contentHeight);

        int yBase = 0;
        for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
        {
            const std::u32string& line = lines[lineIndex];
            const int lineWidth = measureLineWidth(font, line, options);

            int xCursor = 0;
            if (options.alignment == Alignment::Center && outputWidth > lineWidth)
            {
                xCursor = (outputWidth - lineWidth) / 2;
            }
            else if (options.alignment == Alignment::Right && outputWidth > lineWidth)
            {
                xCursor = outputWidth - lineWidth;
            }

            bool firstGlyph = true;
            for (char32_t cp : line)
            {
                if (cp == U'\t')
                {
                    cp = U' ';
                }

                const GlyphPattern* glyph = font.findGlyph(cp);
                if (glyph == nullptr && options.useFallbackGlyphForUnknownChars)
                {
                    glyph = font.findFallbackGlyph();
                }

                if (glyph == nullptr)
                {
                    continue;
                }

                if (!firstGlyph)
                {
                    xCursor += (cp == U' ') ? font.spacing.wordSpacing : font.spacing.letterSpacing;
                }

                const std::vector<int> bounds = computeOpaqueColumnBounds(
                    *glyph,
                    options.trimTrailingTransparentColumns);

                const int xStart = bounds[0];
                const int xEnd = bounds[1];

                for (int gy = 0; gy < glyph->height; ++gy)
                {
                    for (int gx = xStart; gx < xEnd; ++gx)
                    {
                        const GlyphCell& cell = glyph->getCell(gx, gy);
                        const int outX = xCursor + (gx - xStart);
                        const int outY = yBase + gy;

                        if (cell.transparent)
                        {
                            if (!options.transparentSpaces)
                            {
                                builder.setEmpty(outX, outY, chooseCellStyle(
                                    cell,
                                    *glyph,
                                    font,
                                    overrideStyle,
                                    options));
                            }

                            continue;
                        }

                        builder.setGlyph(
                            outX,
                            outY,
                            cell.glyph,
                            chooseCellStyle(cell, *glyph, font, overrideStyle, options));
                    }
                }

                xCursor += std::max(0, xEnd - xStart);
                firstGlyph = false;
            }

            yBase += font.glyphHeight;
            if (lineIndex + 1 < lines.size())
            {
                yBase += font.spacing.lineSpacing;
            }
        }

        return builder.build();
    }
}

namespace PseudoFont
{
    const char* toString(const LoadWarningCode code)
    {
        switch (code)
        {
        case LoadWarningCode::None:
            return "None";
        case LoadWarningCode::MissingOptionalPropertyIgnored:
            return "MissingOptionalPropertyIgnored";
        case LoadWarningCode::DuplicateGlyphReplaced:
            return "DuplicateGlyphReplaced";
        case LoadWarningCode::MissingFallbackGlyph:
            return "MissingFallbackGlyph";
        case LoadWarningCode::NonUniformGlyphSize:
            return "NonUniformGlyphSize";
        default:
            return "Unknown";
        }
    }

    std::string formatLoadError(const LoadResult& result)
    {
        if (result.success)
        {
            return {};
        }

        return result.errorMessage;
    }

    LoadResult loadFromFile(const std::string& filePath)
    {
        return loadFromFile(filePath, {});
    }

    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options)
    {
        LoadResult result;

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            result.errorMessage = "Could not open pseudo font file: " + filePath;
            return result;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        const std::vector<std::string> lines = splitLogicalLines(buffer.str());
        if (lines.empty())
        {
            result.errorMessage = "Pseudo font file is empty: " + filePath;
            return result;
        }

        FontDefinition font;
        font.sourcePath = filePath;

        std::size_t lineIndex = 0;
        while (lineIndex < lines.size() && trim(lines[lineIndex]).empty())
        {
            ++lineIndex;
        }

        if (lineIndex >= lines.size() || !iequals(trim(lines[lineIndex]), "pfont1"))
        {
            result.errorMessage = "Pseudo font file must begin with header line: pfont1";
            return result;
        }

        ++lineIndex;

        bool inGlyph = false;
        char32_t currentCodePoint = U'\0';
        std::vector<std::u32string> currentRows;
        std::optional<Style> currentGlyphStyle;

        auto flushGlyph = [&]() -> bool
            {
                if (!inGlyph)
                {
                    return true;
                }

                if (currentRows.empty())
                {
                    result.errorMessage = "Glyph block has no rows.";
                    return false;
                }

                GlyphPattern glyph;
                glyph.height = static_cast<int>(currentRows.size());
                glyph.width = 0;

                for (const std::u32string& row : currentRows)
                {
                    glyph.width = std::max(glyph.width, static_cast<int>(row.size()));
                }

                if (glyph.width <= 0 || glyph.height <= 0)
                {
                    result.errorMessage = "Glyph block has invalid dimensions.";
                    return false;
                }

                glyph.defaultStyle = currentGlyphStyle;
                glyph.cells.resize(static_cast<std::size_t>(glyph.width * glyph.height));

                for (int y = 0; y < glyph.height; ++y)
                {
                    const std::u32string& row = currentRows[static_cast<std::size_t>(y)];
                    for (int x = 0; x < glyph.width; ++x)
                    {
                        GlyphCell cell;
                        if (x < static_cast<int>(row.size()))
                        {
                            const char32_t cp = row[static_cast<std::size_t>(x)];
                            if (cp == font.transparentMarker)
                            {
                                cell.transparent = true;
                                cell.glyph = U' ';
                            }
                            else
                            {
                                cell.transparent = false;
                                cell.glyph = cp;
                            }
                        }
                        else
                        {
                            cell.transparent = true;
                            cell.glyph = U' ';
                        }

                        glyph.cells[static_cast<std::size_t>(y * glyph.width + x)] = std::move(cell);
                    }
                }

                const auto existing = font.glyphs.find(currentCodePoint);
                if (existing != font.glyphs.end())
                {
                    addWarning(
                        result,
                        LoadWarningCode::DuplicateGlyphReplaced,
                        "Duplicate glyph definition replaced for code point.");
                }

                font.glyphs[currentCodePoint] = std::move(glyph);

                if (!font.hasUniformGlyphSize)
                {
                    font.glyphWidth = font.glyphs[currentCodePoint].width;
                    font.glyphHeight = font.glyphs[currentCodePoint].height;
                    font.hasUniformGlyphSize = true;
                }
                else
                {
                    const GlyphPattern& stored = font.glyphs[currentCodePoint];
                    if ((stored.width != font.glyphWidth || stored.height != font.glyphHeight) &&
                        options.requireUniformGlyphSize)
                    {
                        result.errorMessage = "Pseudo font contains non-uniform glyph sizes while uniform sizing is required.";
                        return false;
                    }

                    if (stored.width != font.glyphWidth || stored.height != font.glyphHeight)
                    {
                        addWarning(
                            result,
                            LoadWarningCode::NonUniformGlyphSize,
                            "Pseudo font contains non-uniform glyph sizes.");
                    }
                }

                inGlyph = false;
                currentCodePoint = U'\0';
                currentRows.clear();
                currentGlyphStyle.reset();
                return true;
            };

        for (; lineIndex < lines.size(); ++lineIndex)
        {
            std::string line = lines[lineIndex];
            if (options.trimTrailingCarriageReturn && !line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            line = trim(line);
            if (line.empty() || startsWith(line, "#"))
            {
                continue;
            }

            if (startsWith(line, "[glyph ") && line.back() == ']')
            {
                if (!flushGlyph())
                {
                    return result;
                }

                std::string token = line.substr(7, line.size() - 8);
                token = trim(token);

                char32_t codePoint = U'\0';
                if (!parseCodePointToken(token, codePoint))
                {
                    result.errorMessage = "Invalid glyph identifier: " + token;
                    return result;
                }

                inGlyph = true;
                currentCodePoint = codePoint;
                currentRows.clear();
                currentGlyphStyle.reset();
                continue;
            }

            if (iequals(line, "[endglyph]"))
            {
                if (!flushGlyph())
                {
                    return result;
                }

                continue;
            }

            const std::size_t equalsPos = line.find('=');
            if (equalsPos == std::string::npos)
            {
                result.errorMessage = "Invalid pseudo font line (expected key=value): " + line;
                return result;
            }

            const std::string key = trim(line.substr(0, equalsPos));
            const std::string value = trim(line.substr(equalsPos + 1));

            if (inGlyph)
            {
                if (iequals(key, "row"))
                {
                    currentRows.push_back(UnicodeConversion::utf8ToU32(value));
                    continue;
                }

                if (startsWith(key, "style."))
                {
                    if (!options.allowGlyphStyleOverrides)
                    {
                        continue;
                    }

                    Style style = currentGlyphStyle.value_or(Style{});
                    if (!applyStyleProperty(style, key.substr(6), value))
                    {
                        result.errorMessage = "Invalid glyph style property: " + key + "=" + value;
                        return result;
                    }

                    currentGlyphStyle = style;
                    continue;
                }

                result.errorMessage = "Unsupported glyph property: " + key;
                return result;
            }

            if (iequals(key, "name"))
            {
                font.name = value;
                continue;
            }

            if (iequals(key, "glyph_width"))
            {
                if (!parseInt(value, font.glyphWidth))
                {
                    result.errorMessage = "Invalid glyph_width value: " + value;
                    return result;
                }

                if (font.glyphWidth < 0)
                {
                    result.errorMessage = "glyph_width must be >= 0.";
                    return result;
                }

                continue;
            }

            if (iequals(key, "glyph_height"))
            {
                if (!parseInt(value, font.glyphHeight))
                {
                    result.errorMessage = "Invalid glyph_height value: " + value;
                    return result;
                }

                if (font.glyphHeight < 0)
                {
                    result.errorMessage = "glyph_height must be >= 0.";
                    return result;
                }

                continue;
            }

            if (iequals(key, "letter_spacing"))
            {
                if (!parseInt(value, font.spacing.letterSpacing))
                {
                    result.errorMessage = "Invalid letter_spacing value: " + value;
                    return result;
                }

                continue;
            }

            if (iequals(key, "word_spacing"))
            {
                if (!parseInt(value, font.spacing.wordSpacing))
                {
                    result.errorMessage = "Invalid word_spacing value: " + value;
                    return result;
                }

                continue;
            }

            if (iequals(key, "line_spacing"))
            {
                if (!parseInt(value, font.spacing.lineSpacing))
                {
                    result.errorMessage = "Invalid line_spacing value: " + value;
                    return result;
                }

                continue;
            }

            if (iequals(key, "fallback"))
            {
                if (!parseCodePointToken(value, font.fallbackCodePoint))
                {
                    result.errorMessage = "Invalid fallback code point: " + value;
                    return result;
                }

                continue;
            }

            if (iequals(key, "transparent"))
            {
                if (!parseCodePointToken(value, font.transparentMarker))
                {
                    result.errorMessage = "Invalid transparent marker: " + value;
                    return result;
                }

                continue;
            }

            if (startsWith(key, "default_style."))
            {
                Style style = font.defaultStyle.value_or(Style{});
                if (!applyStyleProperty(style, key.substr(14), value))
                {
                    result.errorMessage = "Invalid default_style property: " + key + "=" + value;
                    return result;
                }

                font.defaultStyle = style;
                continue;
            }

            addWarning(
                result,
                LoadWarningCode::MissingOptionalPropertyIgnored,
                "Unknown top-level property ignored: " + key);
        }

        if (!flushGlyph())
        {
            return result;
        }

        if (font.name.empty())
        {
            font.name = "PseudoFont";
        }

        if (font.glyphs.empty())
        {
            result.errorMessage = "Pseudo font contains no glyphs.";
            return result;
        }

        if (font.findFallbackGlyph() == nullptr)
        {
            addWarning(
                result,
                LoadWarningCode::MissingFallbackGlyph,
                "Fallback glyph is not defined; unknown characters may be skipped.");
        }

        if (options.requireUniformGlyphSize)
        {
            for (const auto& entry : font.glyphs)
            {
                const GlyphPattern& glyph = entry.second;
                if (glyph.width != font.glyphWidth || glyph.height != font.glyphHeight)
                {
                    result.errorMessage = "Pseudo font failed uniform glyph size validation.";
                    return result;
                }
            }
        }

        result.font = std::move(font);
        result.success = true;
        return result;
    }

    bool tryLoadFromFile(const std::string& filePath, FontDefinition& outFont)
    {
        return tryLoadFromFile(filePath, outFont, {});
    }

    bool tryLoadFromFile(const std::string& filePath, FontDefinition& outFont, const LoadOptions& options)
    {
        const LoadResult result = loadFromFile(filePath, options);
        if (!result.success)
        {
            outFont = {};
            return false;
        }

        outFont = result.font;
        return true;
    }

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text)
    {
        return generateTextObject(font, utf8Text, RenderOptions{});
    }

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text,
        const RenderOptions& options)
    {
        const std::u32string text = UnicodeConversion::utf8ToU32(utf8Text);

        std::vector<std::u32string> lines;
        std::u32string currentLine;

        for (char32_t cp : text)
        {
            if (cp == U'\r')
            {
                continue;
            }

            if (cp == U'\n')
            {
                lines.push_back(currentLine);
                currentLine.clear();
                continue;
            }

            currentLine.push_back(cp);
        }

        lines.push_back(currentLine);
        return buildTextObject(font, lines, std::nullopt, options);
    }

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text,
        const Style& overrideStyle)
    {
        return generateTextObject(font, utf8Text, overrideStyle, RenderOptions{});
    }

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text,
        const Style& overrideStyle,
        const RenderOptions& options)
    {
        const std::u32string text = UnicodeConversion::utf8ToU32(utf8Text);

        std::vector<std::u32string> lines;
        std::u32string currentLine;

        for (char32_t cp : text)
        {
            if (cp == U'\r')
            {
                continue;
            }

            if (cp == U'\n')
            {
                lines.push_back(currentLine);
                currentLine.clear();
                continue;
            }

            currentLine.push_back(cp);
        }

        lines.push_back(currentLine);
        return buildTextObject(font, lines, overrideStyle, options);
    }
}
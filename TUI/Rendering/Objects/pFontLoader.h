#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

namespace PseudoFont
{
    enum class Alignment
    {
        Left,
        Center,
        Right
    };

    struct GlyphCell
    {
        char32_t glyph = U' ';
        bool transparent = true;
        std::optional<Style> style;
    };

    struct GlyphPattern
    {
        int width = 0;
        int height = 0;
        std::vector<GlyphCell> cells;
        std::optional<Style> defaultStyle;

        bool isValid() const
        {
            return width > 0 &&
                height > 0 &&
                static_cast<int>(cells.size()) == width * height;
        }

        const GlyphCell& getCell(int x, int y) const
        {
            return cells[static_cast<std::size_t>(y * width + x)];
        }
    };

    struct SpacingRules
    {
        int letterSpacing = 1;
        int wordSpacing = 1;
        int lineSpacing = 0;
    };

    struct FontDefinition
    {
        using GlyphMap = std::unordered_map<char32_t, GlyphPattern>;

        std::string name;
        std::string sourcePath;

        GlyphMap glyphs;

        int glyphWidth = 0;
        int glyphHeight = 0;
        bool hasUniformGlyphSize = false;

        SpacingRules spacing;
        std::optional<Style> defaultStyle;

        char32_t transparentMarker = U'.';
        char32_t fallbackCodePoint = U'?';

        bool isLoaded() const
        {
            return !glyphs.empty();
        }

        const GlyphPattern* findGlyph(char32_t codePoint) const
        {
            const auto it = glyphs.find(codePoint);
            if (it == glyphs.end())
            {
                return nullptr;
            }

            return &it->second;
        }

        const GlyphPattern* findFallbackGlyph() const
        {
            return findGlyph(fallbackCodePoint);
        }
    };

    enum class LoadWarningCode
    {
        None,
        MissingOptionalPropertyIgnored,
        DuplicateGlyphReplaced,
        MissingFallbackGlyph,
        NonUniformGlyphSize
    };

    struct LoadWarning
    {
        LoadWarningCode code = LoadWarningCode::None;
        std::string message;

        bool isValid() const
        {
            return code != LoadWarningCode::None;
        }
    };

    struct LoadOptions
    {
        bool requireUniformGlyphSize = true;
        bool allowGlyphStyleOverrides = true;
        bool trimTrailingCarriageReturn = true;
    };

    struct LoadResult
    {
        FontDefinition font;
        bool success = false;
        std::vector<LoadWarning> warnings;
        std::string errorMessage;

        bool hasFont() const
        {
            return font.isLoaded();
        }
    };

    struct RenderOptions
    {
        Alignment alignment = Alignment::Left;
        std::size_t targetWidth = 0;

        bool useFallbackGlyphForUnknownChars = true;
        bool transparentSpaces = true;
        bool preserveGlyphStyles = true;
        bool trimTrailingTransparentColumns = true;
    };

    LoadResult loadFromFile(const std::string& filePath);
    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options);

    bool tryLoadFromFile(const std::string& filePath, FontDefinition& outFont);
    bool tryLoadFromFile(const std::string& filePath, FontDefinition& outFont, const LoadOptions& options);

    std::string formatLoadError(const LoadResult& result);
    const char* toString(LoadWarningCode code);

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text);

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text,
        const RenderOptions& options);

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text,
        const Style& overrideStyle);

    TextObject generateTextObject(
        const FontDefinition& font,
        std::string_view utf8Text,
        const Style& overrideStyle,
        const RenderOptions& options);
}
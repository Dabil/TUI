#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

class AsciiBanner
{
public:
    enum class FontKind
    {
        FIGlet,
        Toilet
    };

    enum class ComposeMode
    {
        FullWidth,
        Kern
    };

    enum class Alignment
    {
        Left,
        Center,
        Right
    };

    struct RenderOptions
    {
        ComposeMode composeMode = ComposeMode::Kern;
        Alignment alignment = Alignment::Left;
        std::size_t targetWidth = 0;

        bool replaceHardBlankWithSpace = true;
        bool trimTrailingSpaces = true;
        bool useFallbackGlyphForUnknownChars = true;

        // Preserved for compatibility with previous behavior.
        // It now controls how generated TextObject cells are built:
        // true  -> spaces become Empty cells
        // false -> spaces become explicit glyph cells
        bool transparentSpaces = true;
    };

public:
    AsciiBanner() = default;

    void setFigletFontDirectory(const std::filesystem::path& path);
    void setToiletFontDirectory(const std::filesystem::path& path);

    const std::filesystem::path& getFigletFontDirectory() const;
    const std::filesystem::path& getToiletFontDirectory() const;

    bool loadFont(FontKind kind, const std::string& fontNameWithoutExtension);
    bool loadFontFile(const std::filesystem::path& fontFile);

    bool hasFontLoaded() const;

    std::string getLoadedFontName() const;
    std::filesystem::path getLoadedFontPath() const;
    FontKind getLoadedFontKind() const;

    int getFontHeight() const;

public:
    std::string render(std::string_view text) const;
    std::string render(std::string_view text, const RenderOptions& options) const;

    std::vector<std::string> renderLines(std::string_view text) const;
    std::vector<std::string> renderLines(std::string_view text, const RenderOptions& options) const;

    TextObject generateTextObject(std::string_view text) const;
    TextObject generateTextObject(std::string_view text, const RenderOptions& options) const;

    TextObject generateTextObject(std::string_view text, const Style& style) const;
    TextObject generateTextObject(
        std::string_view text,
        const Style& style,
        const RenderOptions& options) const;

private:
    struct FontData
    {
        std::string name;
        std::filesystem::path path;
        FontKind kind = FontKind::FIGlet;

        char hardBlank = ' ';

        int height = 0;
        int baseline = 0;
        int maxLength = 0;

        int oldLayout = 0;
        int commentLines = 0;
        int printDirection = 0;
        int fullLayout = 0;
        int codetagCount = 0;

        std::unordered_map<int, std::vector<std::string>> glyphs;
    };

private:
    std::filesystem::path m_figletFontDirectory;
    std::filesystem::path m_toiletFontDirectory;

    FontData m_font;
    bool m_hasLoadedFont = false;

private:
    static std::string trimRight(const std::string& s);
    static std::string trimEndMarker(const std::string& s, char endMarker);

    static std::vector<std::string> splitLines(std::string_view text);

    static std::filesystem::path buildFontPath(
        FontKind kind,
        const std::filesystem::path& figletDir,
        const std::filesystem::path& toiletDir,
        const std::string& fontNameWithoutExtension);

    static int getMinimumLeadingSpaces(const std::vector<std::string>& glyph);
    static int getMinimumTrailingSpaces(const std::vector<std::string>& lines);

    const std::vector<std::string>* findGlyph(int codepoint) const;
    const std::vector<std::string>& getFallbackGlyph() const;

    std::vector<std::string> renderSingleLine(
        std::string_view text,
        const RenderOptions& options) const;

    static void appendGlyphFullWidth(
        std::vector<std::string>& outputLines,
        const std::vector<std::string>& glyph);

    static void appendGlyphKern(
        std::vector<std::string>& outputLines,
        const std::vector<std::string>& glyph);

    static std::vector<std::string> alignLines(
        const std::vector<std::string>& lines,
        Alignment alignment,
        std::size_t targetWidth);

    static std::string joinLines(
        const std::vector<std::string>& lines,
        bool trimTrailingSpaces);

    static int measureRenderedLineWidth(std::string_view line);

    static void appendRenderedLineToBuilder(
        TextObjectBuilder& builder,
        int y,
        std::string_view line,
        bool transparentSpaces,
        const std::optional<Style>& style);
};
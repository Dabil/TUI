// Rendering/Objects/XpArtLoader.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

namespace XpArtLoader
{
    enum class FileType
    {
        Auto,
        Unknown,
        Xp
    };

    enum class LoadWarningCode
    {
        None,
        InputWasAlreadyDecompressed,
        TransparentCellsSkipped,
        MultipleLayersFlattened,
        ExtraTrailingBytesIgnored,
        LegacyVersionHeaderDetected,
        LayerSizeMismatchClipped
    };

    struct SourcePosition
    {
        int x = -1;
        int y = -1;

        bool isValid() const
        {
            return x >= 0 && y >= 0;
        }
    };

    struct LoadWarning
    {
        LoadWarningCode code = LoadWarningCode::None;
        std::string message;
        std::size_t byteOffset = 0;
        SourcePosition sourcePosition;

        bool isValid() const
        {
            return code != LoadWarningCode::None;
        }
    };

    struct RgbColor
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;

        bool operator==(const RgbColor& other) const
        {
            return red == other.red && green == other.green && blue == other.blue;
        }

        bool operator!=(const RgbColor& other) const
        {
            return !(*this == other);
        }
    };

    struct CellData
    {
        std::uint32_t glyph = 0;
        RgbColor foreground;
        RgbColor background;

        bool hasTransparentBackground() const;
    };

    struct LayerData
    {
        int width = 0;
        int height = 0;
        std::vector<CellData> cells;

        bool isValid() const;
        bool inBounds(int x, int y) const;
        const CellData* tryGetCell(int x, int y) const;
    };

    // ParsedDocument is the raw parsed XP payload representation.
    // It reflects file structure closely and is intentionally kept separate
    // from the retained runtime-ready XP document model below.
    struct ParsedDocument
    {
        int formatVersion = 0;
        bool usesLegacyLayerCountHeader = false;
        int canvasWidth = 0;
        int canvasHeight = 0;
        std::vector<LayerData> layers;

        bool isValid() const;
    };

    struct ParseResult
    {
        ParsedDocument document;
        bool success = false;
        bool inputWasCompressed = false;
        bool inputWasAlreadyDecompressed = false;

        std::size_t bytesConsumed = 0;
        std::string errorMessage;

        bool hasParseFailure = false;
        std::size_t failingByteOffset = 0;
        SourcePosition firstFailingPosition;
    };

    // XpLayerCell / XpLayer / XpDocument form the retained XP model.
    //
    // Responsibilities:
    // - ParsedDocument:
    //     raw parsed file data and parse-time metadata.
    // - XpDocument:
    //     retained, runtime-ready layered XP content that preserves layer order
    //     and per-cell authored data for future visibility toggles, diagnostics,
    //     alternate merge policies, and animation/frame workflows.
    // - TextObject:
    //     current flattened runtime import target used by the existing placement
    //     and rendering pipeline.
    struct XpLayerCell
    {
        std::uint32_t glyph = 0;
        RgbColor foreground;
        RgbColor background;

        bool hasTransparentBackground() const;
    };

    struct XpLayer
    {
        int width = 0;
        int height = 0;
        bool visible = true;
        std::vector<XpLayerCell> cells;

        bool isValid() const;
        bool inBounds(int x, int y) const;
        const XpLayerCell* tryGetCell(int x, int y) const;
    };

    struct XpDocument
    {
        int width = 0;
        int height = 0;
        int formatVersion = 0;
        bool usesLegacyLayerCountHeader = false;
        std::vector<XpLayer> layers;

        bool isValid() const;
        int getLayerCount() const;
    };

    struct LoadOptions
    {
        FileType fileType = FileType::Auto;

        bool allowCompressedInput = true;
        bool allowAlreadyDecompressedInput = true;

        // When true, retained XP layers are flattened into TextObject output,
        // which remains the current runtime import path.
        //
        // When false, the loader still succeeds and preserves a retained
        // XpDocument in LoadResult, but does not build a flattened TextObject.
        bool flattenLayers = true;

        bool treatMagentaBackgroundAsTransparent = true;
        bool visibleTransparentBaseCellsUseBlackBackground = true;
        bool strictLayerSizeValidation = true;

        std::optional<Style> baseStyle;
    };

    struct LoadResult
    {
        TextObject object;
        XpDocument retainedDocument;

        bool success = false;
        bool hasRetainedDocument = false;

        FileType detectedFileType = FileType::Unknown;
        int resolvedWidth = 0;
        int resolvedHeight = 0;
        int resolvedLayerCount = 0;
        int parsedFormatVersion = 0;

        bool inputWasCompressed = false;
        bool inputWasAlreadyDecompressed = false;
        bool usedLayerFlattening = false;

        std::vector<LoadWarning> warnings;

        bool hasParseFailure = false;
        std::size_t failingByteOffset = 0;
        SourcePosition firstFailingPosition;

        std::string errorMessage;
    };

    FileType detectFileType(const std::string& filePath);

    ParseResult parseDecompressedPayload(std::string_view bytes);

    // Explicit conversion boundary:
    // ParsedDocument -> retained XpDocument
    XpDocument buildRetainedDocument(const ParsedDocument& document);

    LoadResult loadFromFile(const std::string& filePath);
    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options);
    LoadResult loadFromFile(const std::string& filePath, const Style& style);

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject);
    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const LoadOptions& options);
    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const Style& style);

    LoadResult loadFromBytes(std::string_view bytes, const LoadOptions& options = {});

    // Existing convenience path preserved:
    // ParsedDocument -> retained XpDocument -> flattened TextObject
    LoadResult buildTextObject(const ParsedDocument& document, const LoadOptions& options = {});

    // New retained-model flattening entry point:
    // XpDocument -> flattened TextObject
    LoadResult buildTextObject(const XpDocument& document, const LoadOptions& options = {});

    bool hasWarning(const LoadResult& result, LoadWarningCode code);
    const LoadWarning* getWarningByCode(const LoadResult& result, LoadWarningCode code);

    std::string formatLoadError(const LoadResult& result);
    std::string formatLoadSuccess(const LoadResult& result);

    const char* toString(FileType fileType);
    const char* toString(LoadWarningCode warningCode);
}
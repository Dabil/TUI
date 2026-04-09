#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
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
        LayerSizeMismatchClipped,
        HiddenLayersSkipped,
        NonDefaultCompositeModeUsed
    };

    enum class XpCompositeMode
    {
        Phase45BCompatible,
        StrictOpaqueOverwrite
    };

    enum class XpVisibleLayerMode
    {
        UseDocumentVisibility,
        ForceAllLayersVisible,
        UseExplicitVisibleLayerList
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
            return red == other.red &&
                green == other.green &&
                blue == other.blue;
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

    struct XpLayerMetadata
    {
        int sourceLayerIndex = -1;
        int sourceWidth = 0;
        int sourceHeight = 0;

        bool matchedCanvasSize = false;
        bool clippingOccurredDuringFlatten = false;

        bool encounteredTransparentBackgroundCells = false;
        bool encounteredVisibleGlyphsOnTransparentBackground = false;

        bool visibilityUsedForFlattening = true;
        XpCompositeMode compositeModeUsed = XpCompositeMode::Phase45BCompatible;
    };

    struct XpDocumentMetadata
    {
        int canvasWidth = 0;
        int canvasHeight = 0;
        int layerCount = 0;
        int parsedFormatVersion = 0;

        bool retainedPathAvailable = false;
        bool flattenPathUsed = false;

        bool inputWasCompressed = false;
        bool inputWasAlreadyDecompressed = false;

        XpCompositeMode compositeModeUsed = XpCompositeMode::Phase45BCompatible;
    };

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
        XpLayerMetadata metadata;

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
        XpDocumentMetadata metadata;

        bool isValid() const;
        int getLayerCount() const;
    };

    struct XpFrameOverrides
    {
        std::optional<int> durationMilliseconds;
        std::optional<XpCompositeMode> compositeMode;
        std::optional<XpVisibleLayerMode> visibleLayerMode;
        std::vector<int> explicitVisibleLayerIndices;

        bool isEmpty() const;
        bool usesExplicitVisibleLayerList() const;
        bool isValidForDocument(const XpDocument* document) const;
    };

    struct XpSequenceMetadata
    {
        std::optional<int> defaultFrameDurationMilliseconds;
        std::optional<XpCompositeMode> defaultCompositeMode;
        std::optional<XpVisibleLayerMode> defaultVisibleLayerMode;
        std::vector<int> defaultExplicitVisibleLayerIndices;

        std::string sourceManifestPath;
        std::string sequenceLabel;

        bool isEmpty() const;
        bool usesExplicitVisibleLayerList() const;
        bool isValidForDocument(const XpDocument* document) const;
    };

    struct XpFrame
    {
        int frameIndex = 0;
        std::string label;
        std::string sourcePath;
        std::shared_ptr<XpDocument> document;
        XpFrameOverrides overrides;

        bool isValid() const;
        bool hasDocument() const;
        bool hasLabel() const;
        bool hasSourcePath() const;
        const XpDocument* getDocument() const;
        XpDocument* getDocument();

        std::optional<int> resolveDurationMilliseconds(
            const XpSequenceMetadata& sequenceMetadata) const;

        XpCompositeMode resolveCompositeMode(
            const XpSequenceMetadata& sequenceMetadata) const;

        XpVisibleLayerMode resolveVisibleLayerMode(
            const XpSequenceMetadata& sequenceMetadata) const;

        std::vector<int> resolveExplicitVisibleLayerIndices(
            const XpSequenceMetadata& sequenceMetadata) const;
    };

    struct XpSequence
    {
        std::vector<XpFrame> frames;
        XpSequenceMetadata metadata;

        bool isValid() const;
        int getFrameCount() const;
        bool hasUniqueFrameIndices() const;
        bool hasContiguousFrameIndicesStartingAtZero() const;
        bool areFramesStoredInFrameIndexOrder() const;
        void sortFramesByFrameIndex();
        const XpFrame* tryGetFrame(int frameOrdinal) const;
        XpFrame* tryGetFrame(int frameOrdinal);
        const XpFrame* findFrameByIndex(int frameIndex) const;
        XpFrame* findFrameByIndex(int frameIndex);
        const XpFrame* getDefaultFrame() const;
        XpFrame* getDefaultFrame();
    };

    struct LoadOptions
    {
        FileType fileType = FileType::Auto;

        bool allowCompressedInput = true;
        bool allowAlreadyDecompressedInput = true;
        bool flattenLayers = true;

        bool treatMagentaBackgroundAsTransparent = true;
        bool visibleTransparentBaseCellsUseBlackBackground = true;
        bool strictLayerSizeValidation = true;

        XpCompositeMode compositeMode = XpCompositeMode::Phase45BCompatible;

        std::optional<Style> baseStyle;
    };

    struct LoadResult
    {
        TextObject object;
        XpDocument retainedDocument;
        XpSequence retainedSequence;

        bool success = false;
        bool hasRetainedDocument = false;
        bool hasRetainedSequence = false;

        FileType detectedFileType = FileType::Unknown;
        int resolvedWidth = 0;
        int resolvedHeight = 0;
        int resolvedLayerCount = 0;
        int resolvedFrameCount = 0;
        int parsedFormatVersion = 0;

        bool inputWasCompressed = false;
        bool inputWasAlreadyDecompressed = false;
        bool usedLayerFlattening = false;

        XpCompositeMode compositeModeUsed = XpCompositeMode::Phase45BCompatible;
        int visibleLayerCount = 0;
        int hiddenLayerCount = 0;

        std::vector<LoadWarning> warnings;

        bool hasParseFailure = false;
        std::size_t failingByteOffset = 0;
        SourcePosition firstFailingPosition;

        std::string errorMessage;
    };

    FileType detectFileType(const std::string& filePath);

    ParseResult parseDecompressedPayload(std::string_view bytes);

    XpDocument buildRetainedDocument(const ParsedDocument& document);

    XpFrame buildRetainedFrame(
        const ParsedDocument& document,
        int frameIndex = 0,
        const std::string& label = std::string());

    XpSequence buildRetainedSequence(const ParsedDocument& document);

    XpSequence buildRetainedSequence(
        const XpDocument& document,
        int frameIndex = 0,
        const std::string& label = std::string());

    XpSequence buildRetainedSequence(const XpFrame& frame);

    LoadResult loadFromFile(const std::string& filePath);
    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options);
    LoadResult loadFromFile(const std::string& filePath, const Style& style);

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject);
    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const LoadOptions& options);
    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const Style& style);

    LoadResult loadFromBytes(std::string_view bytes, const LoadOptions& options = {});

    LoadResult buildTextObject(const ParsedDocument& document, const LoadOptions& options = {});
    LoadResult buildTextObject(const XpDocument& document, const LoadOptions& options = {});
    LoadResult buildTextObject(const XpFrame& frame, const LoadOptions& options = {});
    LoadResult buildTextObject(const XpSequence& sequence, const LoadOptions& options = {});

    bool hasWarning(const LoadResult& result, LoadWarningCode code);
    const LoadWarning* getWarningByCode(const LoadResult& result, LoadWarningCode code);

    const XpLayer* tryGetLayer(const XpDocument& document, int layerIndex);
    const XpLayerMetadata* tryGetLayerMetadata(const XpDocument& document, int layerIndex);
    const XpFrame* tryGetFrame(const XpSequence& sequence, int frameIndex);

    std::string formatLoadError(const LoadResult& result);
    std::string formatLoadSuccess(const LoadResult& result);
    std::string formatRetainedDocumentSummary(const LoadResult& result);
    std::string formatRetainedLayerSummary(const LoadResult& result, int layerIndex);
    std::string formatRetainedSequenceSummary(const LoadResult& result);

    const char* toString(FileType fileType);
    const char* toString(LoadWarningCode warningCode);
    const char* toString(XpCompositeMode compositeMode);
    const char* toString(XpVisibleLayerMode visibleLayerMode);
}

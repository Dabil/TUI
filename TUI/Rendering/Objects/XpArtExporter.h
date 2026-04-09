#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Rendering/Objects/TextObject.h"
#include "Rendering/Objects/TextObjectExporter.h"
#include "Rendering/Objects/XpArtLoader.h"

namespace XpArtExporter
{
    struct XpColor
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
    };

    struct XpCell
    {
        std::uint32_t glyph = 0;
        XpColor foreground;
        XpColor background;
    };

    struct XpLayer
    {
        int width = 0;
        int height = 0;
        std::vector<XpCell> cells;

        bool isValid() const;
    };

    struct XpDocument
    {
        int formatVersion = 1;
        int canvasWidth = 0;
        int canvasHeight = 0;
        std::vector<XpLayer> layers;

        bool isValid() const;
    };

    enum class RetainedExportMode
    {
        LayeredXp,
        FlattenedXp,
        SequenceContainer,
        FramePerFile
    };

    struct RetainedExportOptions
    {
        RetainedExportMode mode = RetainedExportMode::SequenceContainer;
        TextObjectExporter::SaveOptions xpSaveOptions;

        XpArtLoader::XpCompositeMode flattenCompositeMode =
            XpArtLoader::XpCompositeMode::Phase45BCompatible;

        bool includeHiddenLayers = true;
        bool allowHiddenLayerVisibilityLoss = false;
        bool allowFrameMetadataLoss = false;

        std::string frameFileSeparator = "_frame_";
        int frameNumberWidth = 4;
    };

    struct FrameFileRecord
    {
        int frameIndex = 0;
        std::string label;
        std::string path;
    };

    struct RetainedExportResult
    {
        TextObjectExporter::SaveResult saveResult;
        std::vector<FrameFileRecord> frameFiles;
    };

    bool buildDocument(
        const TextObject& object,
        const TextObjectExporter::SaveOptions& options,
        TextObjectExporter::SaveResult& ioResult,
        XpDocument& outDocument);

    bool buildDocument(
        const XpArtLoader::XpDocument& document,
        const RetainedExportOptions& options,
        TextObjectExporter::SaveResult& ioResult,
        XpDocument& outDocument);

    bool serializeDocument(
        const XpDocument& document,
        TextObjectExporter::SaveResult& ioResult,
        std::string& outPayloadBytes);

    bool compressSerializedDocument(
        std::string_view payloadBytes,
        TextObjectExporter::SaveResult& ioResult,
        std::string& outCompressedBytes);

    bool exportToBytes(
        const TextObject& object,
        const TextObjectExporter::SaveOptions& options,
        TextObjectExporter::SaveResult& ioResult);

    bool exportToBytes(
        const XpArtLoader::XpDocument& document,
        const RetainedExportOptions& options,
        TextObjectExporter::SaveResult& ioResult);

    bool exportSequenceToBytes(
        const XpArtLoader::XpSequence& sequence,
        const RetainedExportOptions& options,
        TextObjectExporter::SaveResult& ioResult);

    bool saveToFile(
        const XpArtLoader::XpDocument& document,
        const std::string& filePath,
        const RetainedExportOptions& options,
        TextObjectExporter::SaveResult& outResult);

    bool saveSequenceToFile(
        const XpArtLoader::XpSequence& sequence,
        const std::string& filePath,
        const RetainedExportOptions& options,
        TextObjectExporter::SaveResult& outResult);

    bool saveSequenceToFrameFiles(
        const XpArtLoader::XpSequence& sequence,
        const std::string& baseFilePath,
        const RetainedExportOptions& options,
        RetainedExportResult& outResult);

    const char* toString(RetainedExportMode mode);
}
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Rendering/Composition/ObjectSource.h"
#include "Rendering/Composition/Placement.h"
#include "Rendering/Composition/RegionRegistry.h"
#include "Rendering/Composition/WritePolicy.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Style.h"

namespace Assets
{
    class AssetLibrary;
}

namespace Composition
{
    struct SourcePlacementResult
    {
        PlacementResult placement;
        ResolvedObjectSource source;
        bool success = false;

        bool hasObject() const
        {
            return source.hasObject();
        }
    };

    class PageComposer
    {
    public:
        struct ScreenTemplateLoadResult
        {
            TextObject object;
            bool success = false;
            std::string errorMessage;
        };

        using ScreenTemplateLoader =
            std::function<ScreenTemplateLoadResult(std::string_view filename)>;

    public:
        PageComposer() = default;
        explicit PageComposer(ScreenBuffer& target);

        void resize(int width, int height);
        int getWidth() const;
        int getHeight() const;

        void setTarget(ScreenBuffer& target);
        void detachTarget();
        bool hasTarget() const;

        ScreenBuffer* tryGetTarget();
        const ScreenBuffer* tryGetTarget() const;

        ScreenBuffer& getTarget();
        const ScreenBuffer& getTarget() const;

        ScreenBuffer captureBuffer() const;
        std::u32string renderToU32String() const;
        std::string renderToUtf8String() const;

        void clear(const Style& style = Style());

        void setScreenTemplateLoader(ScreenTemplateLoader loader);
        bool hasScreenTemplateLoader() const;

        bool createScreen(std::string_view filename);
        bool loadScreen(const TextObject& object);
        bool loadScreen(
            const TextObject& object,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle = std::nullopt);

        bool createRegion(int x, int y, int width, int height, std::string_view name);
        bool hasRegion(std::string_view name) const;
        const NamedRegion* getRegion(std::string_view name) const;
        NamedRegion* getRegion(std::string_view name);
        void clearRegions();

        Rect getFullScreenRegion() const;

        void setAssetLibrary(Assets::AssetLibrary& assetLibrary);
        void detachAssetLibrary();
        bool hasAssetLibrary() const;

        Assets::AssetLibrary* tryGetAssetLibrary();
        const Assets::AssetLibrary* tryGetAssetLibrary() const;

        void setFrames(std::vector<TextObject> frames);
        void clearFrames();
        std::size_t getFrameCount() const;
        bool hasFrame(int frameIndex) const;
        const TextObject* getFrame(int frameIndex) const;

        SourcePlacementResult placeSource(
            const ObjectSource& source,
            int x,
            int y,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeSource(
            const ObjectSource& source,
            const Rect& region,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeSourceInRegion(
            const ObjectSource& source,
            std::string_view regionName,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeSourceAligned(
            const ObjectSource& source,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeAsset(
            std::string_view assetName,
            int x,
            int y,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeAsset(
            std::string_view assetName,
            const Rect& region,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeAssetAligned(
            std::string_view assetName,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeFrame(
            int frameIndex,
            int x,
            int y,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeFrame(
            int frameIndex,
            const Rect& region,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeFrameAligned(
            int frameIndex,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeSequenceFrame(
            std::string_view assetName,
            int frameIndex,
            int x,
            int y,
            const XpArtLoader::XpFrameConversionOptions& frameOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeSequenceFrame(
            std::string_view assetName,
            int frameIndex,
            const Rect& region,
            const Alignment& alignment,
            const XpArtLoader::XpFrameConversionOptions& frameOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeSequenceFrameAligned(
            std::string_view assetName,
            int frameIndex,
            const Alignment& alignment,
            const XpArtLoader::XpFrameConversionOptions& frameOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeXpDocument(
            const XpArtLoader::XpDocument& document,
            int x,
            int y,
            const XpArtLoader::LoadOptions& loadOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeXpFrame(
            const XpArtLoader::XpFrame& frame,
            int x,
            int y,
            const XpArtLoader::XpFrameConversionOptions& frameOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeXpDocumentAligned(
            const XpArtLoader::XpDocument& document,
            const Alignment& alignment,
            const XpArtLoader::LoadOptions& loadOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeXpFrameAligned(
            const XpArtLoader::XpFrame& frame,
            const Alignment& alignment,
            const XpArtLoader::XpFrameConversionOptions& frameOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        Point writeObject(
            const TextObject& object,
            int x,
            int y,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle = std::nullopt);

        Point writeSolidObject(
            const TextObject& object,
            int x,
            int y,
            const std::optional<Style>& overrideStyle = std::nullopt);

        Point writeVisibleObject(
            const TextObject& object,
            int x,
            int y,
            const std::optional<Style>& overrideStyle = std::nullopt);

        Point writeGlyphsOnly(
            const TextObject& object,
            int x,
            int y,
            const std::optional<Style>& overrideStyle = std::nullopt);

        Point writeStyleMask(
            const TextObject& object,
            int x,
            int y,
            const std::optional<Style>& overrideStyle = std::nullopt);

        Point writeStyleBlock(
            const TextObject& object,
            int x,
            int y,
            const std::optional<Style>& overrideStyle = std::nullopt);

        PlacementResult writeObject(
            const TextObject& object,
            const Rect& region,
            const Alignment& alignment,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeSolidObject(
            const TextObject& object,
            const Rect& region,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeVisibleObject(
            const TextObject& object,
            const Rect& region,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeGlyphsOnly(
            const TextObject& object,
            const Rect& region,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeStyleMask(
            const TextObject& object,
            const Rect& region,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeStyleBlock(
            const TextObject& object,
            const Rect& region,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeObjectInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeSolidObjectInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeVisibleObjectInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeGlyphsOnlyInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeStyleMaskInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeStyleBlockInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeObjectAligned(
            const TextObject& object,
            const Alignment& alignment,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeSolidObjectAligned(
            const TextObject& object,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeVisibleObjectAligned(
            const TextObject& object,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeGlyphsOnlyAligned(
            const TextObject& object,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeStyleMaskAligned(
            const TextObject& object,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult writeStyleBlockAligned(
            const TextObject& object,
            const Alignment& alignment,
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        // Backward-compatible placement aliases
        Point placeObject(
            const TextObject& object,
            int x,
            int y,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        PlacementResult placeObject(
            const TextObject& object,
            const Rect& region,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        PlacementResult placeObjectInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        void writeText(int x, int y, std::u32string_view text, const Style& style);
        void writeText(
            int x,
            int y,
            std::u32string_view text,
            const std::optional<Style>& styleOverride = std::nullopt);

        void writeTextUtf8(int x, int y, std::string_view utf8Text, const Style& style);
        void writeTextUtf8(
            int x,
            int y,
            std::string_view utf8Text,
            const std::optional<Style>& styleOverride = std::nullopt);

        void writeTextBlock(int x, int y, std::u32string_view block, const Style& style);
        void writeTextBlock(
            int x,
            int y,
            std::u32string_view block,
            const std::optional<Style>& styleOverride = std::nullopt);

        void writeTextBlockUtf8(int x, int y, std::string_view utf8Block, const Style& style);
        void writeTextBlockUtf8(
            int x,
            int y,
            std::string_view utf8Block,
            const std::optional<Style>& styleOverride = std::nullopt);

    private:
        Point drawObjectAt(
            const TextObject& object,
            int x,
            int y,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle);

        static PlacementResult makeUnresolvedPlacementResult(
            const TextObject& object,
            const Alignment& alignment);

        SourcePlacementResult placeResolvedSource(
            const ResolvedObjectSource& resolvedSource,
            int x,
            int y,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle);

        SourcePlacementResult placeResolvedSource(
            const ResolvedObjectSource& resolvedSource,
            const Rect& region,
            const Alignment& alignment,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle,
            bool clampToRegion);

        static SourcePlacementResult makeFailedSourcePlacement(
            const ResolvedObjectSource& resolvedSource,
            const Alignment* alignment = nullptr);

        void writeSegmentedLine(
            int x,
            int y,
            std::u32string_view line,
            const std::optional<Style>& styleOverride);

        static std::u32string extractFirstLine(std::u32string_view text);
        static std::vector<std::u32string> splitLines(std::u32string_view text);

        void synchronizeTarget();

    private:
        ScreenBuffer* m_target = nullptr;
        Assets::AssetLibrary* m_assetLibrary = nullptr;
        ScreenBuffer m_composedBuffer;
        RegionRegistry m_regions;
        std::vector<TextObject> m_frames;
        ScreenTemplateLoader m_screenTemplateLoader;
    };
}

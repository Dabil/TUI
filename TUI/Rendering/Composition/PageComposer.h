#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>

#include "Rendering/Composition/ObjectSource.h"
#include "Rendering/Composition/Placement.h"
#include "Rendering/Composition/RegionRegistry.h"
#include "Rendering/Composition/WritePolicy.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Diagnostics/PageCompositionDiagnostics.h"
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

        struct PlacementSpec
        {
            enum class Mode
            {
                Point,
                Region,
                RegionName,
                FullScreenAligned
            };

            Mode mode = Mode::Point;

            Point point;
            Rect region;
            std::string regionName;
            Alignment alignment = Align::topLeft();
            bool clampToRegion = false;

            static PlacementSpec at(int x, int y)
            {
                PlacementSpec spec;
                spec.mode = Mode::Point;
                spec.point = { x, y };
                return spec;
            }

            static PlacementSpec inRegion(
                const Rect& regionValue,
                const Alignment& alignmentValue,
                bool clamp = false)
            {
                PlacementSpec spec;
                spec.mode = Mode::Region;
                spec.region = regionValue;
                spec.alignment = alignmentValue;
                spec.clampToRegion = clamp;
                return spec;
            }

            static PlacementSpec inNamedRegion(
                std::string_view name,
                const Alignment& alignmentValue,
                bool clamp = false)
            {
                PlacementSpec spec;
                spec.mode = Mode::RegionName;
                spec.regionName = std::string(name);
                spec.alignment = alignmentValue;
                spec.clampToRegion = clamp;
                return spec;
            }

            static PlacementSpec fullScreen(
                const Alignment& alignmentValue,
                bool clamp = false)
            {
                PlacementSpec spec;
                spec.mode = Mode::FullScreenAligned;
                spec.alignment = alignmentValue;
                spec.clampToRegion = clamp;
                return spec;
            }
        };

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
        bool createRegion(const Rect& rect, std::string_view name);

        bool createFullScreenRegion(std::string_view name);

        bool createCenteredRegion(int width, int height, std::string_view name);
        bool createCenteredRegion(
            const Rect& container,
            int width,
            int height,
            std::string_view name);
        bool createCenteredRegion(
            std::string_view containerRegionName,
            int width,
            int height,
            std::string_view name);

        bool hasRegion(std::string_view name) const;
        const NamedRegion* getRegion(std::string_view name) const;
        NamedRegion* getRegion(std::string_view name);
        void clearRegions();

        Rect getFullScreenRegion() const;

        Rect peekTop(const Rect& rect, int height) const;
        Rect peekBottom(const Rect& rect, int height) const;
        Rect peekLeft(const Rect& rect, int width) const;
        Rect peekRight(const Rect& rect, int width) const;

        Rect peekTop(std::string_view regionName, int height) const;
        Rect peekBottom(std::string_view regionName, int height) const;
        Rect peekLeft(std::string_view regionName, int width) const;
        Rect peekRight(std::string_view regionName, int width) const;

        Rect remainderBelow(const Rect& source, int consumedHeight) const;
        Rect remainderAbove(const Rect& source, int consumedHeight) const;
        Rect remainderRightOf(const Rect& source, int consumedWidth) const;
        Rect remainderLeftOf(const Rect& source, int consumedWidth) const;

        Rect remainderBelow(std::string_view sourceRegionName, int consumedHeight) const;
        Rect remainderAbove(std::string_view sourceRegionName, int consumedHeight) const;
        Rect remainderRightOf(std::string_view sourceRegionName, int consumedWidth) const;
        Rect remainderLeftOf(std::string_view sourceRegionName, int consumedWidth) const;

        std::pair<Rect, Rect> splitTop(const Rect& source, int height) const;
        std::pair<Rect, Rect> splitBottom(const Rect& source, int height) const;
        std::pair<Rect, Rect> splitLeft(const Rect& source, int width) const;
        std::pair<Rect, Rect> splitRight(const Rect& source, int width) const;

        std::pair<Rect, Rect> splitTop(std::string_view sourceRegionName, int height) const;
        std::pair<Rect, Rect> splitBottom(std::string_view sourceRegionName, int height) const;
        std::pair<Rect, Rect> splitLeft(std::string_view sourceRegionName, int width) const;
        std::pair<Rect, Rect> splitRight(std::string_view sourceRegionName, int width) const;

        std::vector<Rect> splitGrid(const Rect& source, int rows, int cols) const;
        std::vector<Rect> splitGrid(std::string_view sourceRegionName, int rows, int cols) const;

        Rect gridCell(const Rect& source, int rows, int cols, int row, int col) const;
        Rect gridCell(std::string_view sourceRegionName, int rows, int cols, int row, int col) const;

        std::vector<Rect> gridRow(const Rect& source, int rows, int cols, int row) const;
        std::vector<Rect> gridRow(std::string_view sourceRegionName, int rows, int cols, int row) const;

        std::vector<Rect> gridColumn(const Rect& source, int rows, int cols, int col) const;
        std::vector<Rect> gridColumn(std::string_view sourceRegionName, int rows, int cols, int col) const;

        bool registerGrid(
            const Rect& source,
            int rows,
            int cols,
            const std::vector<std::string>& names);

        bool registerGrid(
            std::string_view sourceRegionName,
            int rows,
            int cols,
            const std::vector<std::string>& names);

        bool registerGrid(
            const Rect& source,
            int rows,
            int cols,
            std::string_view namePrefix);

        bool registerGrid(
            std::string_view sourceRegionName,
            int rows,
            int cols,
            std::string_view namePrefix);

        bool registerGridArea(
            const Rect& source,
            int rows,
            int cols,
            const std::vector<std::string>& areaNames);

        bool registerGridArea(
            std::string_view sourceRegionName,
            int rows,
            int cols,
            const std::vector<std::string>& areaNames);

        bool registerGridArea(
            const Rect& source,
            int rows,
            int cols,
            std::initializer_list<std::string> areaNames);

        bool registerGridArea(
            std::string_view sourceRegionName,
            int rows,
            int cols,
            std::initializer_list<std::string> areaNames);

        bool createInsetRegion(const Rect& source, int inset, std::string_view name);
        bool createInsetRegion(std::string_view sourceRegionName, int inset, std::string_view name);

        bool createInsetRegion(
            const Rect& source,
            int left,
            int top,
            int right,
            int bottom,
            std::string_view name);

        bool createInsetRegion(
            std::string_view sourceRegionName,
            int left,
            int top,
            int right,
            int bottom,
            std::string_view name);

        bool insetAll(
            const std::vector<std::string>& sourceNames,
            int inset,
            const std::vector<std::string>& destNames);

        bool insetAll(
            const std::vector<std::string>& sourceNames,
            int left,
            int top,
            int right,
            int bottom,
            const std::vector<std::string>& destNames);

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

        void setDiagnostics(PageCompositionDiagnostics& diagnostics);
        void detachDiagnostics();
        bool hasDiagnostics() const;

        PageCompositionDiagnostics* tryGetDiagnostics();
        const PageCompositionDiagnostics* tryGetDiagnostics() const;

        void beginFrame(
            int frameIndex,
            std::string_view channel = {},
            std::string_view tag = {});
        void endFrame();
        void clearFrameContext();

        const PageCompositionDiagnostics::FrameContext& frameContext() const;

        std::uint64_t computeDeterministicSignature() const;
        bool verifyDeterministicSignature(std::uint64_t expectedSignature) const;
        std::uint64_t lastDeterministicSignature() const;

        int centerX(int contentWidth) const;
        int centerY(int contentHeight) const;
        Point centerInFullScreen(const Size& contentSize) const;
        Point anchorInFullScreen(AnchorPoint anchor) const;

        int centerX(std::string_view regionName, int contentWidth) const;
        int centerY(std::string_view regionName, int contentHeight) const;
        Point centerInRegion(std::string_view regionName, const Size& contentSize) const;
        Point anchorInRegion(std::string_view regionName, AnchorPoint anchor) const;

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

        SourcePlacementResult placeSource(
            const ObjectSource& source,
            const PlacementSpec& placement,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

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

        SourcePlacementResult placeActiveFrame(
            int x,
            int y,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeActiveFrame(
            const Rect& region,
            const Alignment& alignment,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeActiveFrameAligned(
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

        SourcePlacementResult placeActiveSequenceFrame(
            std::string_view assetName,
            int x,
            int y,
            const XpArtLoader::XpFrameConversionOptions& frameOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

        SourcePlacementResult placeActiveSequenceFrame(
            std::string_view assetName,
            const Rect& region,
            const Alignment& alignment,
            const XpArtLoader::XpFrameConversionOptions& frameOptions = {},
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt,
            bool clampToRegion = false);

        SourcePlacementResult placeActiveSequenceFrameAligned(
            std::string_view assetName,
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

        PlacementResult writeObject(
            const TextObject& object,
            const PlacementSpec& placement,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle = std::nullopt);

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

        PlacementResult placeObject(
            const TextObject& object,
            const PlacementSpec& placement,
            const WritePolicy& writePolicy = WritePresets::visibleObject(),
            const std::optional<Style>& overrideStyle = std::nullopt);

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

        int resolveActiveFrameIndex() const;

        void recordOperation(
            PageCompositionDiagnostics::OperationKind operation,
            std::string_view operationName,
            const Rect* requestedRegion = nullptr,
            const Rect* resolvedRegion = nullptr,
            const Point* origin = nullptr,
            const Size* contentSize = nullptr,
            const Alignment* alignment = nullptr,
            const WritePolicy* writePolicy = nullptr,
            bool usedAlignment = false,
            bool usedOverrideStyle = false,
            bool clampRequested = false,
            bool clamped = false,
            bool success = true,
            std::string_view regionName = {},
            std::string_view detail = {},
            std::string_view errorMessage = {});

        void recordSourcePlacement(
            PageCompositionDiagnostics::OperationKind operation,
            std::string_view operationName,
            const ResolvedObjectSource& resolvedSource,
            const Rect* requestedRegion,
            const SourcePlacementResult& result,
            const WritePolicy& writePolicy,
            bool usedAlignment,
            bool usedOverrideStyle,
            bool clampRequested,
            std::string_view regionName = {});

        void refreshDeterministicSignature();
        void synchronizeTarget();

    private:
        ScreenBuffer* m_target = nullptr;
        Assets::AssetLibrary* m_assetLibrary = nullptr;
        PageCompositionDiagnostics* m_diagnostics = nullptr;
        ScreenBuffer m_composedBuffer;
        RegionRegistry m_regions;
        std::vector<TextObject> m_frames;
        ScreenTemplateLoader m_screenTemplateLoader;
        PageCompositionDiagnostics::FrameContext m_frameContext;
    };
}
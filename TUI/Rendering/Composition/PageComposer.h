#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
    class PageComposer
    {
    public:
        using DiagnosticsContext = PageCompositionDiagnostics;

        using ScreenTemplateLoader =
            std::function<std::optional<TextObject>(
                std::string_view,
                const Assets::AssetLibrary*)>;

        struct PlacementSpec
        {
            enum class Mode
            {
                Point,
                Region,
                NamedRegion,
                FullScreen
            };

            Mode mode = Mode::Point;
            Point point;
            Rect region;
            std::string regionName;
            Alignment alignment = Align::topLeft();
            bool clampToRegion = false;

            static PlacementSpec at(int x, int y);
            static PlacementSpec inRegion(
                const Rect& regionValue,
                const Alignment& alignmentValue = Align::topLeft(),
                bool clamp = false);
            static PlacementSpec inNamedRegion(
                std::string_view name,
                const Alignment& alignmentValue = Align::topLeft(),
                bool clamp = false);
            static PlacementSpec inFullScreen(
                const Alignment& alignmentValue = Align::topLeft(),
                bool clamp = false);
        };

        struct Frame
        {
            std::string name;
            Rect bounds;
        };

        struct FrameContext
        {
            std::optional<std::string> activeFrameName;
            Rect bounds;
        };

    public:
        PageComposer();
        explicit PageComposer(ScreenBuffer& target);

        // ---------------------------------------------------------------------
        // Target / buffer lifecycle
        // ---------------------------------------------------------------------

        void setTarget(ScreenBuffer& target);
        void detachTarget();

        bool hasTarget() const;
        ScreenBuffer* tryGetTarget();
        const ScreenBuffer* tryGetTarget() const;

        ScreenBuffer& getTarget();
        const ScreenBuffer& getTarget() const;

        void resize(int width, int height);
        void clear(const Style& style = Style());

        int getWidth() const;
        int getHeight() const;

        TextObject captureBuffer() const;
        std::u32string renderToU32String() const;
        std::string renderToUtf8String() const;

        // ---------------------------------------------------------------------
        // Region / layout system
        // ---------------------------------------------------------------------

        void createRegion(std::string_view name, const Rect& rect);
        void replaceRegion(std::string_view name, const Rect& rect);
        void removeRegion(std::string_view name);
        void renameRegion(std::string_view from, std::string_view to);
        void clearRegions();

        bool hasRegion(std::string_view name) const;
        std::optional<Rect> tryGetRegion(std::string_view name) const;
        Rect resolveRegion(std::string_view name) const;

        Rect getFullScreenRegion() const;

        void createFullScreenRegion(std::string_view name);
        void createCenteredRegion(std::string_view name, int width, int height);
        void createCenteredRegion(std::string_view name, const Size& size);
        void createInsetRegion(
            std::string_view name,
            std::string_view parent,
            int inset);
        void createFramedScreenRegions(
            std::string_view outerName,
            std::string_view innerName,
            int thickness);
        void createFramedScreenRegions(
            std::string_view outerName,
            std::string_view innerName,
            int horizontalThickness,
            int verticalThickness);

        void registerGrid(
            std::string_view baseName,
            const Rect& bounds,
            int rows,
            int cols);
        void registerGridArea(
            std::string_view name,
            std::string_view baseName,
            int row,
            int col);

        Rect peekTop(const Rect& base, int height) const;
        Rect peekBottom(const Rect& base, int height) const;
        Rect peekLeft(const Rect& base, int width) const;
        Rect peekRight(const Rect& base, int width) const;

        Rect remainderBelow(const Rect& base, int height) const;
        Rect remainderAbove(const Rect& base, int height) const;
        Rect remainderRightOf(const Rect& base, int width) const;
        Rect remainderLeftOf(const Rect& base, int width) const;

        std::pair<Rect, Rect> splitTop(const Rect& base, int height) const;
        std::pair<Rect, Rect> splitBottom(const Rect& base, int height) const;
        std::pair<Rect, Rect> splitLeft(const Rect& base, int width) const;
        std::pair<Rect, Rect> splitRight(const Rect& base, int width) const;

        std::vector<Rect> splitGrid(const Rect& base, int rows, int cols) const;

        Rect gridCell(const Rect& base, int rows, int cols, int row, int col) const;
        Rect gridRow(const Rect& base, int rows, int row) const;
        Rect gridColumn(const Rect& base, int cols, int col) const;

        // ---------------------------------------------------------------------
        // Core composition API
        // ---------------------------------------------------------------------

        void placeSource(
            const ObjectSource& source,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        void writeObject(
            const TextObject& object,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        void writeText(
            std::string_view text,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        void writeTextBlock(
            std::string_view text,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        void writeAlignedText(
            std::string_view text,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        void writeAlignedTextBlock(
            std::string_view text,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        void writeWrappedText(
            std::string_view text,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        // ---------------------------------------------------------------------
        // Earned sugar layer
        // ---------------------------------------------------------------------

        // Object placement sugar
        void writeAt(
            const TextObject& object,
            int x,
            int y,
            const WritePolicy& policy);

        void writeInRegion(
            const TextObject& object,
            std::string_view regionName,
            const Alignment& alignment,
            const WritePolicy& policy,
            bool clampToRegion = false);

        void writeAligned(
            const TextObject& object,
            const Alignment& alignment,
            const WritePolicy& policy,
            bool clampToRegion = false);

        void writeVisible(
            const TextObject& object,
            const PlacementSpec& placement);

        void writeSolid(
            const TextObject& object,
            const PlacementSpec& placement);

        // Text sugar
        void writeTextInRegion(
            std::string_view text,
            std::string_view regionName,
            const Alignment& alignment,
            const WritePolicy& policy,
            bool clampToRegion = false);

        void writeTextBlockInRegion(
            std::string_view text,
            std::string_view regionName,
            const Alignment& alignment,
            const WritePolicy& policy,
            bool clampToRegion = false);

        void writeWrappedTextInRegion(
            std::string_view text,
            std::string_view regionName,
            const Alignment& alignment,
            const WritePolicy& policy,
            bool clampToRegion = false);

        void writeCenteredText(
            std::string_view text,
            const WritePolicy& policy,
            bool clampToRegion = false);

        // ---------------------------------------------------------------------
        // Asset / template hooks
        // ---------------------------------------------------------------------

        void setAssetLibrary(Assets::AssetLibrary* library);
        void detachAssetLibrary();

        bool hasAssetLibrary() const;
        Assets::AssetLibrary* tryGetAssetLibrary();
        const Assets::AssetLibrary* tryGetAssetLibrary() const;

        void setScreenTemplateLoader(ScreenTemplateLoader loader);
        std::optional<TextObject> loadScreenTemplate(std::string_view name) const;

        void loadScreen(
            const TextObject& screen,
            const PlacementSpec& placement,
            const WritePolicy& policy);

        // ---------------------------------------------------------------------
        // Frame / deterministic context hooks
        // ---------------------------------------------------------------------

        void setFrames(const std::vector<Frame>& frames);
        void clearFrames();
        std::size_t getFrameCount() const;
        bool hasFrame(std::string_view name) const;
        const Frame* tryGetFrame(std::string_view name) const;

        void beginFrame(std::string_view name);
        void endFrame();
        void clearFrameContext();
        const FrameContext& getFrameContext() const;

        // ---------------------------------------------------------------------
        // Diagnostics hooks
        // ---------------------------------------------------------------------

        void setDiagnostics(DiagnosticsContext* ctx);
        void detachDiagnostics();
        bool hasDiagnostics() const;
        DiagnosticsContext* tryGetDiagnostics();
        const DiagnosticsContext* tryGetDiagnostics() const;

        std::uint64_t computeDeterministicSignature() const;
        bool verifyDeterministicSignature(std::uint64_t expected) const;
        std::uint64_t getLastDeterministicSignature() const;

    private:
        Rect resolveRegionOrFullScreen(std::string_view name) const;
        Rect resolvePlacementRegion(const PlacementSpec& placement) const;
        PlacementResult resolvePlacementResult(
            const PlacementSpec& placement,
            const Size& contentSize) const;
        void ensureBufferInitialized() const;
        void refreshFromTarget();
        void synchronizeTarget();

        static std::u32string extractFirstLine(std::u32string_view text);
        static std::vector<std::u32string> splitLines(std::u32string_view text);
        static std::vector<std::u32string> wrapSingleLineToLines(
            std::u32string_view text,
            int maxWidth);
        static std::vector<std::u32string> wrapTextToLines(
            std::u32string_view text,
            int maxWidth);
        static int measureDisplayWidth(std::u32string_view text);
        static std::u32string trimTrailingWrapWhitespace(std::u32string_view text);
        static bool isWrapWhitespaceText(std::u32string_view text);

    private:
        ScreenBuffer* m_target = nullptr;
        ScreenBuffer m_buffer;
        RegionRegistry m_regions;
        Assets::AssetLibrary* m_assetLibrary = nullptr;
        ScreenTemplateLoader m_templateLoader;
        std::vector<Frame> m_frames;
        FrameContext m_frameContext;
        DiagnosticsContext* m_diagnostics = nullptr;
        mutable std::uint64_t m_lastSignature = 0;
    };
}

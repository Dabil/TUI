#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "Rendering/Composition/Placement.h"
#include "Rendering/Composition/RegionRegistry.h"
#include "Rendering/Composition/WritePolicy.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Style.h"

namespace Composition
{
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
        ScreenBuffer m_composedBuffer;
        RegionRegistry m_regions;
        ScreenTemplateLoader m_screenTemplateLoader;
    };
}
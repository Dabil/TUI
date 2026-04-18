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

        void setTarget(ScreenBuffer& target);
        bool hasTarget() const;

        ScreenBuffer* tryGetTarget();
        const ScreenBuffer* tryGetTarget() const;

        ScreenBuffer& getTarget();
        const ScreenBuffer& getTarget() const;

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

    private:
        Point drawObjectAt(
            const TextObject& object,
            int x,
            int y,
            const WritePolicy& writePolicy,
            const std::optional<Style>& overrideStyle);

    private:
        ScreenBuffer* m_target = nullptr;
        RegionRegistry m_regions;
        ScreenTemplateLoader m_screenTemplateLoader;
    };
}
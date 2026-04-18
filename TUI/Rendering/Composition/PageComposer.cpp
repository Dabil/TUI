#include "Rendering/Composition/PageComposer.h"

#include <stdexcept>

namespace Composition
{
    PageComposer::PageComposer(ScreenBuffer& target)
        : m_target(&target)
    {
    }

    void PageComposer::setTarget(ScreenBuffer& target)
    {
        m_target = &target;
    }

    bool PageComposer::hasTarget() const
    {
        return m_target != nullptr;
    }

    ScreenBuffer* PageComposer::tryGetTarget()
    {
        return m_target;
    }

    const ScreenBuffer* PageComposer::tryGetTarget() const
    {
        return m_target;
    }

    ScreenBuffer& PageComposer::getTarget()
    {
        if (m_target == nullptr)
        {
            throw std::runtime_error("PageComposer has no target ScreenBuffer.");
        }

        return *m_target;
    }

    const ScreenBuffer& PageComposer::getTarget() const
    {
        if (m_target == nullptr)
        {
            throw std::runtime_error("PageComposer has no target ScreenBuffer.");
        }

        return *m_target;
    }

    void PageComposer::clear(const Style& style)
    {
        if (m_target == nullptr)
        {
            return;
        }

        m_target->clear(style);
    }

    void PageComposer::setScreenTemplateLoader(ScreenTemplateLoader loader)
    {
        m_screenTemplateLoader = std::move(loader);
    }

    bool PageComposer::hasScreenTemplateLoader() const
    {
        return static_cast<bool>(m_screenTemplateLoader);
    }

    bool PageComposer::createScreen(std::string_view filename)
    {
        if (m_target == nullptr || !m_screenTemplateLoader)
        {
            return false;
        }

        const ScreenTemplateLoadResult loadResult = m_screenTemplateLoader(filename);
        if (!loadResult.success)
        {
            return false;
        }

        return loadScreen(loadResult.object);
    }

    bool PageComposer::loadScreen(const TextObject& object)
    {
        return loadScreen(object, WritePresets::solidObject(), std::nullopt);
    }

    bool PageComposer::loadScreen(
        const TextObject& object,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        if (m_target == nullptr || !object.isLoaded())
        {
            return false;
        }

        object.draw(*m_target, 0, 0, writePolicy, overrideStyle);
        return true;
    }

    bool PageComposer::createRegion(
        int x,
        int y,
        int width,
        int height,
        std::string_view name)
    {
        return m_regions.createRegion(x, y, width, height, name);
    }

    bool PageComposer::hasRegion(std::string_view name) const
    {
        return m_regions.hasRegion(name);
    }

    const NamedRegion* PageComposer::getRegion(std::string_view name) const
    {
        return m_regions.getRegion(name);
    }

    NamedRegion* PageComposer::getRegion(std::string_view name)
    {
        return m_regions.getRegion(name);
    }

    void PageComposer::clearRegions()
    {
        m_regions.clearRegions();
    }

    Point PageComposer::placeObject(
        const TextObject& object,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return drawObjectAt(object, x, y, writePolicy, overrideStyle);
    }

    PlacementResult PageComposer::placeObject(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        PlacementRequest request;
        request.region = region;
        request.contentSize = measureObject(object);
        request.alignment = alignment;
        request.clampToRegion = clampToRegion;

        PlacementResult result = resolvePlacement(request);
        drawObjectAt(
            object,
            result.origin.x,
            result.origin.y,
            writePolicy,
            overrideStyle);

        return result;
    }

    PlacementResult PageComposer::placeObjectInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        const NamedRegion* region = m_regions.getRegion(regionName);
        if (region == nullptr)
        {
            PlacementResult result;
            result.region = makeRect(0, 0, 0, 0);
            result.contentSize = measureObject(object);
            result.alignment = alignment;
            result.origin = Point{};
            result.clamped = false;
            return result;
        }

        return placeObject(
            object,
            region->bounds,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    Point PageComposer::drawObjectAt(
        const TextObject& object,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        Point origin;
        origin.x = x;
        origin.y = y;

        if (m_target == nullptr || !object.isLoaded())
        {
            return origin;
        }

        object.draw(*m_target, x, y, writePolicy, overrideStyle);
        return origin;
    }
}
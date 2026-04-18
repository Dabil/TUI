#include "Rendering/Composition/Placement.h"

#include <algorithm>

#include "Rendering/Objects/TextObject.h"

namespace
{
    int resolveHorizontalOffset(
        int availableWidth,
        int contentWidth,
        Composition::HorizontalAlign horizontalAlign)
    {
        switch (horizontalAlign)
        {
        case Composition::HorizontalAlign::Center:
            return (availableWidth - contentWidth) / 2;

        case Composition::HorizontalAlign::Right:
            return availableWidth - contentWidth;

        case Composition::HorizontalAlign::Left:
        default:
            return 0;
        }
    }

    int resolveVerticalOffset(
        int availableHeight,
        int contentHeight,
        Composition::VerticalAlign verticalAlign)
    {
        switch (verticalAlign)
        {
        case Composition::VerticalAlign::Center:
            return (availableHeight - contentHeight) / 2;

        case Composition::VerticalAlign::Bottom:
            return availableHeight - contentHeight;

        case Composition::VerticalAlign::Top:
        default:
            return 0;
        }
    }

    int clampInt(int value, int minValue, int maxValue)
    {
        if (minValue > maxValue)
        {
            return minValue;
        }

        return std::max(minValue, std::min(value, maxValue));
    }
}

namespace Composition
{
    PlacementResult resolvePlacement(const PlacementRequest& request)
    {
        PlacementResult result;
        result.region = request.region;
        result.contentSize = request.contentSize;
        result.alignment = request.alignment;
        result.clamped = false;

        Point origin = resolveAlignedOrigin(
            request.region,
            request.contentSize,
            request.alignment);

        if (request.clampToRegion)
        {
            const int minX = request.region.position.x;
            const int minY = request.region.position.y;
            const int maxX =
                request.region.position.x +
                std::max(0, request.region.size.width - request.contentSize.width);
            const int maxY =
                request.region.position.y +
                std::max(0, request.region.size.height - request.contentSize.height);

            const int clampedX = clampInt(origin.x, minX, maxX);
            const int clampedY = clampInt(origin.y, minY, maxY);

            result.clamped = (clampedX != origin.x) || (clampedY != origin.y);
            origin.x = clampedX;
            origin.y = clampedY;
        }

        result.origin = origin;
        return result;
    }

    Point resolveAlignedOrigin(
        const Rect& region,
        const Size& contentSize,
        HorizontalAlign horizontalAlign,
        VerticalAlign verticalAlign)
    {
        Point origin;
        origin.x =
            region.position.x +
            resolveHorizontalOffset(region.size.width, contentSize.width, horizontalAlign);
        origin.y =
            region.position.y +
            resolveVerticalOffset(region.size.height, contentSize.height, verticalAlign);
        return origin;
    }

    Point resolveAlignedOrigin(
        const Rect& region,
        const Size& contentSize,
        const Alignment& alignment)
    {
        return resolveAlignedOrigin(
            region,
            contentSize,
            alignment.horizontal,
            alignment.vertical);
    }

    Rect makeRect(int x, int y, int width, int height)
    {
        Rect rect;
        rect.position.x = x;
        rect.position.y = y;
        rect.size.width = width;
        rect.size.height = height;
        return rect;
    }

    Size measureObject(const TextObject& object)
    {
        Size size;
        size.width = object.getWidth();
        size.height = object.getHeight();
        return size;
    }
}
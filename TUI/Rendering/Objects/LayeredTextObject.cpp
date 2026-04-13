#include "Rendering/Objects/LayeredTextObject.h"

#include <algorithm>
#include <utility>

#include "Rendering/Objects/TextObjectBuilder.h"

namespace
{
    void blitTextObject(
        TextObjectBuilder& builder,
        const TextObject& source,
        const int offsetX,
        const int offsetY,
        const std::optional<Style>& overrideStyle)
    {
        for (int y = 0; y < source.getHeight(); ++y)
        {
            for (int x = 0; x < source.getWidth(); ++x)
            {
                const TextObjectCell* cell = source.tryGetCell(x, y);
                if (cell == nullptr)
                {
                    continue;
                }

                if (cell->kind == CellKind::Empty)
                {
                    continue;
                }

                const int destX = offsetX + x;
                const int destY = offsetY + y;

                if (!builder.inBounds(destX, destY))
                {
                    continue;
                }

                const std::optional<Style> styleToApply =
                    overrideStyle.has_value() ? overrideStyle : cell->style;

                switch (cell->kind)
                {
                case CellKind::Glyph:
                    if (cell->width == CellWidth::Two)
                    {
                        builder.setWideGlyph(destX, destY, cell->glyph, styleToApply);
                    }
                    else
                    {
                        builder.setGlyph(destX, destY, cell->glyph, styleToApply);
                    }
                    break;

                case CellKind::WideTrailing:
                case CellKind::CombiningContinuation:
                    builder.setCell(destX, destY, cell->glyph, cell->kind, cell->width, styleToApply);
                    break;

                case CellKind::Empty:
                default:
                    break;
                }
            }
        }
    }
}

namespace Rendering
{
    LayeredTextObject::LayeredTextObject(int width, int height)
        : m_width(width > 0 ? width : 0)
        , m_height(height > 0 ? height : 0)
        , m_loaded(m_width > 0 && m_height > 0)
    {
    }

    void LayeredTextObject::clear()
    {
        m_width = 0;
        m_height = 0;
        m_loaded = false;
        m_layers.clear();
    }

    bool LayeredTextObject::isLoaded() const
    {
        return m_loaded;
    }

    bool LayeredTextObject::isEmpty() const
    {
        return m_layers.empty();
    }

    int LayeredTextObject::getWidth() const
    {
        return m_width;
    }

    int LayeredTextObject::getHeight() const
    {
        return m_height;
    }

    void LayeredTextObject::setDimensions(int width, int height)
    {
        m_width = width > 0 ? width : 0;
        m_height = height > 0 ? height : 0;
        m_loaded = (m_width > 0 && m_height > 0);
    }

    std::size_t LayeredTextObject::getLayerCount() const
    {
        return m_layers.size();
    }

    const std::vector<TextObjectLayer>& LayeredTextObject::getLayers() const
    {
        return m_layers;
    }

    std::vector<TextObjectLayer>& LayeredTextObject::getLayers()
    {
        return m_layers;
    }

    const TextObjectLayer* LayeredTextObject::findLayer(std::string_view name) const
    {
        const auto it = std::find_if(
            m_layers.begin(),
            m_layers.end(),
            [&](const TextObjectLayer& layer)
            {
                return layer.name == name;
            });

        return (it != m_layers.end()) ? &(*it) : nullptr;
    }

    TextObjectLayer* LayeredTextObject::findLayer(std::string_view name)
    {
        const auto it = std::find_if(
            m_layers.begin(),
            m_layers.end(),
            [&](const TextObjectLayer& layer)
            {
                return layer.name == name;
            });

        return (it != m_layers.end()) ? &(*it) : nullptr;
    }

    bool LayeredTextObject::hasLayer(std::string_view name) const
    {
        return findLayer(name) != nullptr;
    }

    bool LayeredTextObject::addLayer(const TextObjectLayer& layer)
    {
        if (layer.name.empty() || hasLayer(layer.name))
        {
            return false;
        }

        m_layers.push_back(layer);
        return true;
    }

    bool LayeredTextObject::addLayer(TextObjectLayer&& layer)
    {
        if (layer.name.empty() || hasLayer(layer.name))
        {
            return false;
        }

        m_layers.push_back(std::move(layer));
        return true;
    }

    bool LayeredTextObject::removeLayer(std::string_view name)
    {
        const auto it = std::find_if(
            m_layers.begin(),
            m_layers.end(),
            [&](const TextObjectLayer& layer)
            {
                return layer.name == name;
            });

        if (it == m_layers.end())
        {
            return false;
        }

        m_layers.erase(it);
        return true;
    }

    void LayeredTextObject::setLayerVisibility(std::string_view name, bool visible)
    {
        TextObjectLayer* layer = findLayer(name);
        if (layer != nullptr)
        {
            layer->visible = visible;
        }
    }

    bool LayeredTextObject::getLayerVisibility(std::string_view name, bool defaultValue) const
    {
        const TextObjectLayer* layer = findLayer(name);
        return (layer != nullptr) ? layer->visible : defaultValue;
    }

    void LayeredTextObject::setLayerOffset(std::string_view name, int offsetX, int offsetY)
    {
        TextObjectLayer* layer = findLayer(name);
        if (layer != nullptr)
        {
            layer->offsetX = offsetX;
            layer->offsetY = offsetY;
        }
    }

    TextObject LayeredTextObject::flattenVisibleOnly() const
    {
        FlattenOptions options;
        options.visibleOnly = true;
        return flatten(options);
    }

    TextObject LayeredTextObject::flattenAllLayers() const
    {
        FlattenOptions options;
        options.visibleOnly = false;
        return flatten(options);
    }

    TextObject LayeredTextObject::flatten(const FlattenOptions& options) const
    {
        return flattenInternal(m_layers, m_width, m_height, options);
    }

    TextObject LayeredTextObject::flattenInternal(
        const std::vector<TextObjectLayer>& layers,
        const int width,
        const int height,
        const FlattenOptions& options)
    {
        if (width <= 0 || height <= 0)
        {
            return TextObject{};
        }

        std::vector<const TextObjectLayer*> sortedLayers;
        sortedLayers.reserve(layers.size());

        for (const TextObjectLayer& layer : layers)
        {
            if (options.visibleOnly && !layer.visible)
            {
                continue;
            }

            sortedLayers.push_back(&layer);
        }

        if (sortedLayers.empty())
        {
            return TextObjectBuilder(width, height).build();
        }

        std::stable_sort(
            sortedLayers.begin(),
            sortedLayers.end(),
            [](const TextObjectLayer* a, const TextObjectLayer* b)
            {
                return a->zIndex < b->zIndex;
            });

        TextObjectBuilder builder(width, height);

        for (const TextObjectLayer* layer : sortedLayers)
        {
            if (layer == nullptr || layer->object.isEmpty())
            {
                continue;
            }

            blitTextObject(
                builder,
                layer->object,
                layer->offsetX,
                layer->offsetY,
                options.overrideStyle);
        }

        return builder.build();
    }
}
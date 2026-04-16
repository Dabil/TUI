#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

namespace Rendering
{
    struct TextObjectLayer
    {
        std::string name;
        int zIndex = 0;
        bool visible = true;
        int offsetX = 0;
        int offsetY = 0;
        TextObject object;

        bool empty() const
        {
            return object.isEmpty();
        }
    };

    class LayeredTextObject
    {
    public:
        struct FlattenOptions
        {
            bool visibleOnly = true;
            std::optional<Style> overrideStyle;
        };

    public:
        LayeredTextObject() = default;
        LayeredTextObject(int width, int height);

        void clear();

        bool isLoaded() const;
        bool isEmpty() const;

        int getWidth() const;
        int getHeight() const;
        void setDimensions(int width, int height);

        std::size_t getLayerCount() const;

        const std::vector<TextObjectLayer>& getLayers() const;
        std::vector<TextObjectLayer>& getLayers();

        const TextObjectLayer* findLayer(std::string_view name) const;
        TextObjectLayer* findLayer(std::string_view name);

        bool hasLayer(std::string_view name) const;
        bool addLayer(const TextObjectLayer& layer);
        bool addLayer(TextObjectLayer&& layer);
        bool removeLayer(std::string_view name);

        void setLayerVisibility(std::string_view name, bool visible);
        bool getLayerVisibility(std::string_view name, bool defaultValue = false) const;

        void setLayerOffset(std::string_view name, int offsetX, int offsetY);

        TextObject flattenVisibleOnly() const;
        TextObject flattenAllLayers() const;
        TextObject flatten(const FlattenOptions& options) const;

    private:
        static TextObject flattenInternal(
            const std::vector<TextObjectLayer>& layers,
            int width,
            int height,
            const FlattenOptions& options);

    private:
        int m_width = 0;
        int m_height = 0;
        bool m_loaded = false;
        std::vector<TextObjectLayer> m_layers;
    };
}
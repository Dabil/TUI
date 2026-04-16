#include "Rendering/Objects/BannerFactory.h"

#include <algorithm>
#include <string>
#include <utility>

#include "Utilities/Unicode/UnicodeConversion.h"

namespace BannerFactory
{
    namespace
    {
        std::string toUtf8(std::u32string_view text)
        {
            return UnicodeConversion::u32ToUtf8(text);
        }

        int minLayerZIndex(const Rendering::LayeredTextObject& layered)
        {
            const std::vector<Rendering::TextObjectLayer>& layers = layered.getLayers();
            if (layers.empty())
            {
                return 0;
            }

            int minValue = layers.front().zIndex;
            for (const Rendering::TextObjectLayer& layer : layers)
            {
                minValue = std::min(minValue, layer.zIndex);
            }

            return minValue;
        }

        std::string makeUniqueLayerName(
            const Rendering::LayeredTextObject& layered,
            std::string_view baseName)
        {
            std::string candidate(baseName);
            if (!layered.hasLayer(candidate))
            {
                return candidate;
            }

            int suffix = 1;
            for (;;)
            {
                candidate = std::string(baseName) + "_" + std::to_string(suffix);
                if (!layered.hasLayer(candidate))
                {
                    return candidate;
                }

                ++suffix;
            }
        }

        Rendering::LayeredTextObject makePFontShadowComposite(
            const Rendering::LayeredTextObject& mainObject,
            const TextObject& shadowObject,
            const int shadowOffsetX,
            const int shadowOffsetY)
        {
            const int width =
                std::max(
                    mainObject.getWidth(),
                    shadowObject.getWidth() + std::max(0, shadowOffsetX));

            const int height =
                std::max(
                    mainObject.getHeight(),
                    shadowObject.getHeight() + std::max(0, shadowOffsetY));

            Rendering::LayeredTextObject layered(width, height);

            Rendering::TextObjectLayer shadowLayer;
            shadowLayer.name = makeUniqueLayerName(mainObject, "shadow");
            shadowLayer.zIndex = minLayerZIndex(mainObject) - 1;
            shadowLayer.visible = true;
            shadowLayer.offsetX = shadowOffsetX;
            shadowLayer.offsetY = shadowOffsetY;
            shadowLayer.object = shadowObject;
            layered.addLayer(std::move(shadowLayer));

            for (const Rendering::TextObjectLayer& sourceLayer : mainObject.getLayers())
            {
                Rendering::TextObjectLayer copiedLayer = sourceLayer;

                if (copiedLayer.name.empty())
                {
                    copiedLayer.name = makeUniqueLayerName(layered, "layer");
                }
                else if (layered.hasLayer(copiedLayer.name))
                {
                    copiedLayer.name = makeUniqueLayerName(layered, copiedLayer.name);
                }

                layered.addLayer(std::move(copiedLayer));
            }

            return layered;
        }
    }

    // -------------------------------------------------------------------------
    // Render option presets
    // -------------------------------------------------------------------------

    AsciiBanner::RenderOptions defaultOptions()
    {
        return AsciiBanner::RenderOptions{};
    }

    AsciiBanner::RenderOptions kernOptions()
    {
        AsciiBanner::RenderOptions options;
        options.composeMode = AsciiBanner::ComposeMode::Kern;
        return options;
    }

    AsciiBanner::RenderOptions fullWidthOptions()
    {
        AsciiBanner::RenderOptions options;
        options.composeMode = AsciiBanner::ComposeMode::FullWidth;
        return options;
    }

    AsciiBanner::RenderOptions centeredOptions(const std::size_t targetWidth)
    {
        AsciiBanner::RenderOptions options;
        options.alignment = AsciiBanner::Alignment::Center;
        options.targetWidth = targetWidth;
        return options;
    }

    AsciiBanner::RenderOptions centeredKernOptions(const std::size_t targetWidth)
    {
        AsciiBanner::RenderOptions options;
        options.composeMode = AsciiBanner::ComposeMode::Kern;
        options.alignment = AsciiBanner::Alignment::Center;
        options.targetWidth = targetWidth;
        return options;
    }

    // -------------------------------------------------------------------------
    // Preferred API: makeBanner(...)
    // AsciiBannerFont UTF-8 overloads
    // -------------------------------------------------------------------------

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text)
    {
        return AsciiBanner::generateTextObject(font, text);
    }

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const AsciiBanner::RenderOptions& options)
    {
        return AsciiBanner::generateTextObject(font, text, options);
    }

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style)
    {
        return AsciiBanner::generateTextObject(font, text, style);
    }

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style,
        const AsciiBanner::RenderOptions& options)
    {
        return AsciiBanner::generateTextObject(font, text, style, options);
    }

    // -------------------------------------------------------------------------
    // Preferred API: makeBanner(...)
    // AsciiBannerFont UTF-32 overloads
    // -------------------------------------------------------------------------

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text)
    {
        return makeBanner(font, toUtf8(text));
    }

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const AsciiBanner::RenderOptions& options)
    {
        return makeBanner(font, toUtf8(text), options);
    }

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& style)
    {
        return makeBanner(font, toUtf8(text), style);
    }

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& style,
        const AsciiBanner::RenderOptions& options)
    {
        return makeBanner(font, toUtf8(text), style, options);
    }

    // -------------------------------------------------------------------------
    // Preferred API: makeBanner(...)
    // pFont UTF-8 overloads
    // -------------------------------------------------------------------------

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::string_view text)
    {
        return PseudoFont::generateTextObject(font, text);
    }

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const PseudoFont::RenderOptions& options)
    {
        return PseudoFont::generateTextObject(font, text, options);
    }

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& overrideStyle)
    {
        return PseudoFont::generateTextObject(font, text, overrideStyle);
    }

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& overrideStyle,
        const PseudoFont::RenderOptions& options)
    {
        return PseudoFont::generateTextObject(font, text, overrideStyle, options);
    }

    // -------------------------------------------------------------------------
    // Preferred API: makeBanner(...)
    // pFont UTF-32 overloads
    // -------------------------------------------------------------------------

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text)
    {
        return makeBanner(font, toUtf8(text));
    }

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const PseudoFont::RenderOptions& options)
    {
        return makeBanner(font, toUtf8(text), options);
    }

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& overrideStyle)
    {
        return makeBanner(font, toUtf8(text), overrideStyle);
    }

    TextObject makeBanner(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& overrideStyle,
        const PseudoFont::RenderOptions& options)
    {
        return makeBanner(font, toUtf8(text), overrideStyle, options);
    }

    // -------------------------------------------------------------------------
    // Layered helpers
    // AsciiBanner synthetic layered helpers
    // -------------------------------------------------------------------------

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle)
    {
        return makeLayeredBannerObject(
            font,
            text,
            layerOneStyle,
            layerTwoStyle,
            defaultOptions());
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle,
        const AsciiBanner::RenderOptions& options)
    {
        const TextObject first = makeBanner(font, text, layerOneStyle, options);
        const TextObject second = makeBanner(font, text, layerTwoStyle, options);

        const int width = std::max(first.getWidth(), second.getWidth());
        const int height = std::max(first.getHeight(), second.getHeight());

        Rendering::LayeredTextObject layered(width, height);

        Rendering::TextObjectLayer layerOne;
        layerOne.name = "layer_one";
        layerOne.zIndex = 0;
        layerOne.visible = true;
        layerOne.object = first;
        layered.addLayer(std::move(layerOne));

        Rendering::TextObjectLayer layerTwo;
        layerTwo.name = "layer_two";
        layerTwo.zIndex = 1;
        layerTwo.visible = true;
        layerTwo.object = second;
        layered.addLayer(std::move(layerTwo));

        return layered;
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle)
    {
        return makeLayeredBannerObject(
            font,
            text,
            layerOneStyle,
            layerTwoStyle,
            defaultOptions());
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle,
        const AsciiBanner::RenderOptions& options)
    {
        return makeLayeredBannerObject(font, toUtf8(text), layerOneStyle, layerTwoStyle, options);
    }

    // -------------------------------------------------------------------------
    // Layered helpers
    // pFont native layered helpers
    // -------------------------------------------------------------------------

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::string_view text)
    {
        return PseudoFont::generateLayeredTextObject(font, text);
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const PseudoFont::LayeredRenderOptions& options)
    {
        return PseudoFont::generateLayeredTextObject(font, text, options);
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& overrideStyle)
    {
        return PseudoFont::generateLayeredTextObject(font, text, overrideStyle);
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& overrideStyle,
        const PseudoFont::LayeredRenderOptions& options)
    {
        return PseudoFont::generateLayeredTextObject(font, text, overrideStyle, options);
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text)
    {
        return makeLayeredBannerObject(font, toUtf8(text));
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const PseudoFont::LayeredRenderOptions& options)
    {
        return makeLayeredBannerObject(font, toUtf8(text), options);
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& overrideStyle)
    {
        return makeLayeredBannerObject(font, toUtf8(text), overrideStyle);
    }

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& overrideStyle,
        const PseudoFont::LayeredRenderOptions& options)
    {
        return makeLayeredBannerObject(font, toUtf8(text), overrideStyle, options);
    }

    // -------------------------------------------------------------------------
    // Shadow effects
    // AsciiBanner only
    // -------------------------------------------------------------------------

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeShadowBanner(
            font,
            text,
            mainStyle,
            shadowStyle,
            defaultOptions(),
            shadowOffsetX,
            shadowOffsetY);
    }

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        const Rendering::LayeredTextObject layered = makeLayeredShadowBannerObject(
            font,
            text,
            mainStyle,
            shadowStyle,
            options,
            shadowOffsetX,
            shadowOffsetY);

        return layered.flattenVisibleOnly();
    }

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeShadowBanner(
            font,
            text,
            mainStyle,
            shadowStyle,
            defaultOptions(),
            shadowOffsetX,
            shadowOffsetY);
    }

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeShadowBanner(
            font,
            toUtf8(text),
            mainStyle,
            shadowStyle,
            options,
            shadowOffsetX,
            shadowOffsetY);
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeLayeredShadowBannerObject(
            font,
            text,
            mainStyle,
            shadowStyle,
            defaultOptions(),
            shadowOffsetX,
            shadowOffsetY);
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        const TextObject mainObject = makeBanner(font, text, mainStyle, options);
        const TextObject shadowObject = makeBanner(font, text, shadowStyle, options);

        const int width = std::max(mainObject.getWidth(), shadowObject.getWidth() + std::max(0, shadowOffsetX));
        const int height = std::max(mainObject.getHeight(), shadowObject.getHeight() + std::max(0, shadowOffsetY));

        Rendering::LayeredTextObject layered(width, height);

        Rendering::TextObjectLayer shadowLayer;
        shadowLayer.name = "shadow";
        shadowLayer.zIndex = 0;
        shadowLayer.visible = true;
        shadowLayer.offsetX = shadowOffsetX;
        shadowLayer.offsetY = shadowOffsetY;
        shadowLayer.object = shadowObject;
        layered.addLayer(std::move(shadowLayer));

        Rendering::TextObjectLayer mainLayer;
        mainLayer.name = "main";
        mainLayer.zIndex = 1;
        mainLayer.visible = true;
        mainLayer.object = mainObject;
        layered.addLayer(std::move(mainLayer));

        return layered;
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeLayeredShadowBannerObject(
            font,
            text,
            mainStyle,
            shadowStyle,
            defaultOptions(),
            shadowOffsetX,
            shadowOffsetY);
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeLayeredShadowBannerObject(
            font,
            toUtf8(text),
            mainStyle,
            shadowStyle,
            options,
            shadowOffsetX,
            shadowOffsetY);
    }

    // -------------------------------------------------------------------------
    // Shadow effects
    // pFont
    // -------------------------------------------------------------------------

    TextObject makeShadowBanner(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        PseudoFont::RenderOptions shadowOptions;
        PseudoFont::LayeredRenderOptions mainOptions;

        const Rendering::LayeredTextObject layered = makeLayeredShadowBannerObject(
            font,
            text,
            mainStyle,
            shadowStyle,
            shadowOptions,
            mainOptions,
            shadowOffsetX,
            shadowOffsetY);

        return layered.flattenVisibleOnly();
    }

    TextObject makeShadowBanner(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const PseudoFont::RenderOptions& options,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        const TextObject mainObject = makeBanner(font, text, mainStyle, options);
        const TextObject shadowObject = makeBanner(font, text, shadowStyle, options);

        const int width = std::max(mainObject.getWidth(), shadowObject.getWidth() + std::max(0, shadowOffsetX));
        const int height = std::max(mainObject.getHeight(), shadowObject.getHeight() + std::max(0, shadowOffsetY));

        Rendering::LayeredTextObject layered(width, height);

        Rendering::TextObjectLayer shadowLayer;
        shadowLayer.name = "shadow";
        shadowLayer.zIndex = 0;
        shadowLayer.visible = true;
        shadowLayer.offsetX = shadowOffsetX;
        shadowLayer.offsetY = shadowOffsetY;
        shadowLayer.object = shadowObject;
        layered.addLayer(std::move(shadowLayer));

        Rendering::TextObjectLayer mainLayer;
        mainLayer.name = "main";
        mainLayer.zIndex = 1;
        mainLayer.visible = true;
        mainLayer.object = mainObject;
        layered.addLayer(std::move(mainLayer));

        return layered.flattenVisibleOnly();
    }

    TextObject makeShadowBanner(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeShadowBanner(
            font,
            toUtf8(text),
            mainStyle,
            shadowStyle,
            shadowOffsetX,
            shadowOffsetY);
    }

    TextObject makeShadowBanner(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const PseudoFont::RenderOptions& options,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeShadowBanner(
            font,
            toUtf8(text),
            mainStyle,
            shadowStyle,
            options,
            shadowOffsetX,
            shadowOffsetY);
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        PseudoFont::RenderOptions shadowOptions;
        PseudoFont::LayeredRenderOptions mainOptions;

        return makeLayeredShadowBannerObject(
            font,
            text,
            mainStyle,
            shadowStyle,
            shadowOptions,
            mainOptions,
            shadowOffsetX,
            shadowOffsetY);
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const PseudoFont::FontDefinition& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const PseudoFont::RenderOptions& shadowOptions,
        const PseudoFont::LayeredRenderOptions& mainOptions,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        const TextObject shadowObject = makeBanner(font, text, shadowStyle, shadowOptions);
        const Rendering::LayeredTextObject mainObject =
            makeLayeredBannerObject(font, text, mainStyle, mainOptions);

        return makePFontShadowComposite(
            mainObject,
            shadowObject,
            shadowOffsetX,
            shadowOffsetY);
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeLayeredShadowBannerObject(
            font,
            toUtf8(text),
            mainStyle,
            shadowStyle,
            shadowOffsetX,
            shadowOffsetY);
    }

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const PseudoFont::FontDefinition& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const PseudoFont::RenderOptions& shadowOptions,
        const PseudoFont::LayeredRenderOptions& mainOptions,
        int shadowOffsetX,
        int shadowOffsetY)
    {
        return makeLayeredShadowBannerObject(
            font,
            toUtf8(text),
            mainStyle,
            shadowStyle,
            shadowOptions,
            mainOptions,
            shadowOffsetX,
            shadowOffsetY);
    }
}
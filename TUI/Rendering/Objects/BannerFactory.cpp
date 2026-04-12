#include "Rendering/Objects/BannerFactory.h"

#include "Utilities/Unicode/UnicodeConversion.h"

namespace BannerFactory
{
    namespace
    {
        std::string toUtf8(std::u32string_view text)
        {
            return UnicodeConversion::u32ToUtf8(text);
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
    // UTF-8 overloads
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
    // UTF-32 overloads
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
    // Layered / shadowed helpers
    // -------------------------------------------------------------------------

    LayeredBanner makeLayeredBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle)
    {
        return makeLayeredBanner(font, text, mainStyle, shadowStyle, defaultOptions());
    }

    LayeredBanner makeLayeredBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options)
    {
        LayeredBanner layered;
        layered.main = makeBanner(font, text, mainStyle, options);
        layered.shadow = makeBanner(font, text, shadowStyle, options);
        return layered;
    }

    LayeredBanner makeLayeredBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle)
    {
        return makeLayeredBanner(font, text, mainStyle, shadowStyle, defaultOptions());
    }

    LayeredBanner makeLayeredBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options)
    {
        return makeLayeredBanner(font, toUtf8(text), mainStyle, shadowStyle, options);
    }
}
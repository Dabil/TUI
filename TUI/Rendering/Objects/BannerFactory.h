#pragma once

#include <cstddef>
#include <string_view>

#include "Assets/Models/AsciiBannerFont.h"
#include "Rendering/Objects/AsciiBanner.h"
#include "Rendering/Objects/LayeredTextObject.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

namespace BannerFactory
{
    // -------------------------------------------------------------------------
    // Render option presets
    // -------------------------------------------------------------------------

    AsciiBanner::RenderOptions defaultOptions();
    AsciiBanner::RenderOptions kernOptions();
    AsciiBanner::RenderOptions fullWidthOptions();
    AsciiBanner::RenderOptions centeredOptions(std::size_t targetWidth);
    AsciiBanner::RenderOptions centeredKernOptions(std::size_t targetWidth);

    // -------------------------------------------------------------------------
    // Preferred API: makeBanner(...)
    // UTF-8 overloads
    // -------------------------------------------------------------------------

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text);

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const AsciiBanner::RenderOptions& options);

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style);

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style,
        const AsciiBanner::RenderOptions& options);

    // -------------------------------------------------------------------------
    // Preferred API: makeBanner(...)
    // UTF-32 overloads
    // -------------------------------------------------------------------------

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text);

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const AsciiBanner::RenderOptions& options);

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& style);

    TextObject makeBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& style,
        const AsciiBanner::RenderOptions& options);

    // -------------------------------------------------------------------------
    // Generic layered helpers
    // -------------------------------------------------------------------------

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle);

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle,
        const AsciiBanner::RenderOptions& options);

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle);

    Rendering::LayeredTextObject makeLayeredBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& layerOneStyle,
        const Style& layerTwoStyle,
        const AsciiBanner::RenderOptions& options);

    // -------------------------------------------------------------------------
    // Shadow effects
    // -------------------------------------------------------------------------

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);

    TextObject makeShadowBanner(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);

    Rendering::LayeredTextObject makeLayeredShadowBannerObject(
        const AsciiBannerFont& font,
        std::u32string_view text,
        const Style& mainStyle,
        const Style& shadowStyle,
        const AsciiBanner::RenderOptions& options,
        int shadowOffsetX = 1,
        int shadowOffsetY = 1);
}
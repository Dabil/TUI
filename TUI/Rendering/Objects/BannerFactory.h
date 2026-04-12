#pragma once

#include <string_view>

#include "Assets/Models/AsciiBannerFont.h"
#include "Rendering/Objects/AsciiBanner.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

namespace BannerFactory
{
    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text);

    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text,
        const AsciiBanner::RenderOptions& options);

    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style);

    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style,
        const AsciiBanner::RenderOptions& options);
}
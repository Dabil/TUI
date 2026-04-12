#include "Rendering/Objects/BannerFactory.h"

namespace BannerFactory
{
    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text)
    {
        return AsciiBanner::generateTextObject(font, text);
    }

    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text,
        const AsciiBanner::RenderOptions& options)
    {
        return AsciiBanner::generateTextObject(font, text, options);
    }

    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style)
    {
        return AsciiBanner::generateTextObject(font, text, style);
    }

    TextObject makeBannerText(
        const AsciiBannerFont& font,
        std::string_view text,
        const Style& style,
        const AsciiBanner::RenderOptions& options)
    {
        return AsciiBanner::generateTextObject(font, text, style, options);
    }
}
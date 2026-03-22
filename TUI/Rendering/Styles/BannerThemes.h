#pragma once

#include "Rendering/Styles/Color.h"
#include "Rendering/Styles/StyleBuilder.h"

namespace BannerThemes
{
    // =========================================================
    // Titles / headings / banner presentation
    // =========================================================

    inline const Style Title =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan));

    inline const Style TitleShadow =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightBlack));

    inline const Style Subtitle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite));

    inline const Style SectionHeader =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow));

    inline const Style InfoBanner =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style SuccessBanner =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Green));

    inline const Style WarningBanner =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
        + style::Bg(Color::FromBasic(Color::Basic::Red));

    inline const Style RetroBanner =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightGreen))
        + style::Bg(Color::FromBasic(Color::Basic::Black));
}
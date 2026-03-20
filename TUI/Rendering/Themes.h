#pragma once

#include "Rendering/Styles/Color.h"
#include "Rendering/Styles/StyleBuilder.h"

namespace Themes
{
    // =========================================================
    // Base surfaces / page backgrounds
    // =========================================================

    inline const Style Background =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style Window =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style Panel =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style AccentSurface =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style RetroTerminal =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightGreen))
        + style::Bg(Color::FromBasic(Color::Basic::Black));


    // =========================================================
    // Neutral UI text styles
    // =========================================================

    inline const Style Text =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite));

    inline const Style MutedText =
        style::Dim
        + style::Fg(Color::FromBasic(Color::Basic::White));

    inline const Style Frame =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite));

    inline const Style Focused =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow));

    inline const Style Selected =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::Black))
        + style::Bg(Color::FromBasic(Color::Basic::BrightWhite));

    inline const Style Disabled =
        style::Dim
        + style::Fg(Color::FromBasic(Color::Basic::White));


    // =========================================================
    // Titles / headings / labels
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

    inline const Style Emphasis =
        style::Bold
        + style::Underline
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan));


    // =========================================================
    // Informational / feedback / status styles
    // =========================================================

    inline const Style Info =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan));

    inline const Style Success =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightGreen));

    inline const Style Warning =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
        + style::Bg(Color::FromBasic(Color::Basic::Red));

    inline const Style Danger =
        style::Bold
        + style::FastBlink
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Red));

    inline const Style Highlight =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::Black))
        + style::Bg(Color::FromBasic(Color::Basic::BrightYellow));

    inline const Style Dimmed =
        style::Dim
        + style::Fg(Color::FromBasic(Color::Basic::White));


    // =========================================================
    // Generic framework demo / object styles
    // =========================================================

    inline const Style Accent =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightMagenta));

    inline const Style DemoObject =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightGreen));

    inline const Style DemoObjectAlt =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan));
}
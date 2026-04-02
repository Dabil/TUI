#pragma once

#include "Rendering/Styles/Color.h"
#include "Rendering/Styles/StyleBuilder.h"

/*
    Can also create styles like this:

    inline Style PanelTitle()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightCyan));
    }

    But then you will have to use them like this:

    buffer.writeString(4, 0, "[ Some Text Here ]", Themes::PanelTitle());

    Personally I prefer the current ANSI retro feel.
*/

namespace Themes
{
    // =========================================================
    // Base surfaces / page backgrounds
    // =========================================================

    inline const Style Background =
          style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Black),
            Color::FromIndexed(0),
            Color::FromRgb(0, 0, 0)));

    inline const Style Window =
        style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Blue),
            Color::FromIndexed(4),
            Color::FromRgb(0, 0, 128)));

    inline const Style Panel =
        style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightWhite),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Black),
            Color::FromIndexed(0),
            Color::FromRgb(0, 0, 0)));

    inline const Style AccentSurface =
        style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightWhite),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Blue),
            Color::FromIndexed(4),
            Color::FromRgb(0, 0, 128)));

    inline const Style RetroTerminal =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightGreen),
            Color::FromIndexed(10),
            Color::FromRgb(0, 255, 0)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Black),
            Color::FromIndexed(0),
            Color::FromRgb(0, 0, 0)));

    // =========================================================
    // Neutral UI text styles
    // =========================================================
    // When using Foreground only styles, make sure you are 
    // placing the glyphs on a cell where the background is 
    // already owned. (ie a cell where the background has already
    // been directly specified) 

    inline const Style PanelTitle =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightWhite),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)));

    inline const Style Text =
        style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightWhite),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)));

    inline const Style MutedText =
        style::Dim
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(7),
            Color::FromRgb(192, 192, 192)));

    inline const Style Frame =
        style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(7),
            Color::FromRgb(192, 192, 192)));

    inline const Style Focused =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightYellow),
            Color::FromIndexed(11),
            Color::FromRgb(255, 255, 0)));

    inline const Style Selected =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Black),
            Color::FromIndexed(0),
            Color::FromRgb(0, 0, 0)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightWhite),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)));

    inline const Style Disabled =
        style::Dim
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(7),
            Color::FromRgb(192, 192, 192)));

    // =========================================================
    // Titles / headings / labels
    // =========================================================

    inline const Style Title =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightCyan),
            Color::FromIndexed(14),
            Color::FromRgb(0, 255, 255)));

    inline const Style TitleShadow =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightBlack),
            Color::FromIndexed(8),
            Color::FromRgb(128, 128, 128)));

    inline const Style Subtitle =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightWhite),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)));

    inline const Style SectionHeader =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightYellow),
            Color::FromIndexed(11),
            Color::FromRgb(255, 255, 0)));

    inline const Style Emphasis =
        style::Bold
        + style::Underline
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightCyan),
            Color::FromIndexed(14),
            Color::FromRgb(0, 255, 255)));

    // =========================================================
    // Informational / feedback / status styles
    // =========================================================

    inline const Style Info =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightCyan),
            Color::FromIndexed(14),
            Color::FromRgb(0, 255, 255)));

    inline const Style Success =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightGreen),
            Color::FromIndexed(10),
            Color::FromRgb(0, 255, 0)));

    inline const Style Warning =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightYellow),
            Color::FromIndexed(11),
            Color::FromRgb(255, 255, 0)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Red),
            Color::FromIndexed(1),
            Color::FromRgb(128, 0, 0)));

    inline const Style Danger =
        style::Bold
        + style::FastBlink
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightWhite),
            Color::FromIndexed(15),
            Color::FromRgb(255, 255, 255)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Red),
            Color::FromIndexed(1),
            Color::FromRgb(128, 0, 0)));

    inline const Style Hightlight =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Black),
            Color::FromIndexed(0),
            Color::FromRgb(0, 0, 0)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightYellow),
            Color::FromIndexed(11),
            Color::FromRgb(255, 255, 0)));


    inline const Style Dimmed =
        style::Dim
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(7),
            Color::FromRgb(192, 192, 192)));

    inline const Style SlowBlink =
        style::SlowBlink
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(7),
            Color::FromRgb(192, 192, 192)));


    inline const Style FastBlink =
        style::FastBlink
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(7),
            Color::FromRgb(192, 192, 192)));

    // =========================================================
    // Generic framework demo / object styles
    // =========================================================

    inline const Style Accent =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightMagenta),
            Color::FromIndexed(13),
            Color::FromRgb(255, 0, 255)));

    inline const Style DemoObject =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightGreen),
            Color::FromIndexed(10),
            Color::FromRgb(0, 255, 0)));

    inline const Style DemoObjectAlt =
        style::Bold
        + style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::BrightCyan),
            Color::FromIndexed(14),
            Color::FromRgb(0, 255, 255)));
}
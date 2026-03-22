#pragma once

#include "Rendering/Styles/StyleBuilder.h"

namespace UIThemes
{
    inline Style NormalText()
    {
        return Style();
    }

    inline Style DisabledText()
    {
        return style::Dim
            + style::Fg(Color::FromBasic(Color::Basic::BrightBlack));
    }

    inline Style Label()
    {
        return style::Fg(Color::FromBasic(Color::Basic::BrightWhite));
    }

    inline Style Value()
    {
        return style::Fg(Color::FromBasic(Color::Basic::BrightCyan));
    }

    inline Style DialogTitle()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
            + style::Bg(Color::FromBasic(Color::Basic::Blue));
    }

    inline Style PanelTitle()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightCyan));
    }

    inline Style StatusBar()
    {
        return style::Fg(Color::FromBasic(Color::Basic::Black))
            + style::Bg(Color::FromBasic(Color::Basic::White));
    }

    inline Style Selection()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::Black))
            + style::Bg(Color::FromBasic(Color::Basic::BrightCyan));
    }

    inline Style Focused()
    {
        return style::Bold
            + style::Underline;
    }

    inline Style ButtonNormal()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
            + style::Bg(Color::FromBasic(Color::Basic::Blue));
    }

    inline Style ButtonHot()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
            + style::Bg(Color::FromBasic(Color::Basic::Blue));
    }

    inline Style ButtonDisabled()
    {
        return style::Dim
            + style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
            + style::Bg(Color::FromBasic(Color::Basic::Black));
    }

    inline Style Border()
    {
        return style::Fg(Color::FromBasic(Color::Basic::BrightBlue));
    }

    inline Style Header()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
            + style::Bg(Color::FromBasic(Color::Basic::BrightBlack));
    }

    inline Style Footer()
    {
        return style::Fg(Color::FromBasic(Color::Basic::White))
            + style::Bg(Color::FromBasic(Color::Basic::Black));
    }

    inline Style Tooltip()
    {
        return style::Fg(Color::FromBasic(Color::Basic::Black))
            + style::Bg(Color::FromBasic(Color::Basic::BrightYellow));
    }

    inline Style ErrorText()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightRed));
    }

    inline Style WarningText()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightYellow));
    }

    inline Style SuccessText()
    {
        return style::Bold
            + style::Fg(Color::FromBasic(Color::Basic::BrightGreen));
    }
}
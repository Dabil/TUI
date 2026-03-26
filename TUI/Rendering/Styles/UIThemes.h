#pragma once

#include "Rendering/Styles/StyleBuilder.h"

namespace UIThemes
{
    inline Style NormalText = Style();

    inline const Style DisabledText =
          style::Dim
        + style::Fg(Color::FromBasic(Color::Basic::BrightBlack));


    inline const Style Label =
          style::Fg(Color::FromBasic(Color::Basic::BrightWhite));
 
    inline const Style Value =
          style::Fg(Color::FromBasic(Color::Basic::BrightCyan));


    inline const Style DialogTitle =
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));
 

    inline const Style PanelTitle =
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan));

    inline const Style StatusBar = 
          style::Fg(Color::FromBasic(Color::Basic::Black))
        + style::Bg(Color::FromBasic(Color::Basic::White));

    inline const Style Selection = 
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::Black))
        + style::Bg(Color::FromBasic(Color::Basic::BrightCyan));

    inline const Style Focused = 
          style::Bold
        + style::Underline;

    inline const Style ButtonNormal = 
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style ButtonHot = 
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style ButtonDisabled = 
          style::Dim
            + style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
            + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style Border =
          style::Fg(Color::FromBasic(Color::Basic::BrightBlue));

    inline const Style Header = 
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::BrightBlack));

    inline const Style Footer = 
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style Tooltip = 
          style::Fg(Color::FromBasic(Color::Basic::Black))
        + style::Bg(Color::FromBasic(Color::Basic::BrightYellow));

    inline const Style ErrorText = 
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightRed));

    inline const Style WarningText = 
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow));

    inline const Style SuccessText = 
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightGreen));
}
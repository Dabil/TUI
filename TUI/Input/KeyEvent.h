#pragma once

#include <cstdint>

namespace Input
{
    enum class KeyCode
    {
        Unknown,

        Character,

        Escape,
        Enter,
        Backspace,
        Tab,
        Space,

        Left,
        Right,
        Up,
        Down,

        Home,
        End,
        PageUp,
        PageDown,
        Insert,
        Delete,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12
    };

    struct KeyModifiers
    {
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
    };

    struct KeyEvent
    {
        KeyCode code = KeyCode::Unknown;
        char32_t character = U'\0';
        KeyModifiers modifiers;
        bool pressed = false;
        std::uint16_t repeatCount = 1;
    };
}
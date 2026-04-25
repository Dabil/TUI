#pragma once

#include <cstdint>
#include <optional>

#include "Input/KeyEvent.h"

namespace Input
{
    struct RawKeyEvent
    {
        std::uint16_t virtualKey = 0;
        char32_t character = U'\0';
        KeyModifiers modifiers;
        bool pressed = false;
        std::uint16_t repeatCount = 1;
    };

    class IRawKeyboardSource
    {
    public:
        virtual ~IRawKeyboardSource() = default;

        virtual bool initialize() = 0;
        virtual void shutdown() = 0;

        virtual std::optional<RawKeyEvent> pollRawKey() = 0;
    };
}
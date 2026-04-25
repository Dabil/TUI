#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "Input/KeyEvent.h"
#include "Input/RawKeyboardSource.h"

namespace Input
{
    class InputManager
    {
    public:
        InputManager();
        explicit InputManager(std::unique_ptr<IRawKeyboardSource> keyboardSource);
        ~InputManager();

        bool initialize();
        void shutdown();

        void poll();

        bool hasEvents() const;
        std::optional<KeyEvent> popEvent();
        std::vector<KeyEvent> drainEvents();

        void clear();

    private:
        KeyEvent normalizeRawKey(const RawKeyEvent& raw) const;

    private:
        std::unique_ptr<IRawKeyboardSource> m_keyboardSource;
        std::vector<KeyEvent> m_events;
        bool m_initialized = false;
    };
}
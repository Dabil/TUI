#include "Input/InputManager.h"

#include "Input/WindowsConsoleKeyboardSource.h"

namespace Input
{
    namespace
    {
        KeyCode mapVirtualKey(std::uint16_t virtualKey, char32_t character)
        {
            if (character != U'\0')
            {
                if (character == U' ')
                {
                    return KeyCode::Space;
                }

                if (character == U'\r' || character == U'\n')
                {
                    return KeyCode::Enter;
                }

                if (character == U'\b')
                {
                    return KeyCode::Backspace;
                }

                if (character == U'\t')
                {
                    return KeyCode::Tab;
                }

                if (character == U'\x1B')
                {
                    return KeyCode::Escape;
                }

                return KeyCode::Character;
            }

            switch (virtualKey)
            {
            case 0x1B: return KeyCode::Escape;
            case 0x0D: return KeyCode::Enter;
            case 0x08: return KeyCode::Backspace;
            case 0x09: return KeyCode::Tab;
            case 0x20: return KeyCode::Space;

            case 0x25: return KeyCode::Left;
            case 0x27: return KeyCode::Right;
            case 0x26: return KeyCode::Up;
            case 0x28: return KeyCode::Down;

            case 0x24: return KeyCode::Home;
            case 0x23: return KeyCode::End;
            case 0x21: return KeyCode::PageUp;
            case 0x22: return KeyCode::PageDown;
            case 0x2D: return KeyCode::Insert;
            case 0x2E: return KeyCode::Delete;

            case 0x70: return KeyCode::F1;
            case 0x71: return KeyCode::F2;
            case 0x72: return KeyCode::F3;
            case 0x73: return KeyCode::F4;
            case 0x74: return KeyCode::F5;
            case 0x75: return KeyCode::F6;
            case 0x76: return KeyCode::F7;
            case 0x77: return KeyCode::F8;
            case 0x78: return KeyCode::F9;
            case 0x79: return KeyCode::F10;
            case 0x7A: return KeyCode::F11;
            case 0x7B: return KeyCode::F12;

            default:
                return KeyCode::Unknown;
            }
        }
    }

    InputManager::InputManager()
        : m_keyboardSource(std::make_unique<WindowsConsoleKeyboardSource>())
    {
    }

    InputManager::InputManager(std::unique_ptr<IRawKeyboardSource> keyboardSource)
        : m_keyboardSource(std::move(keyboardSource))
    {
    }

    InputManager::~InputManager()
    {
        shutdown();
    }

    bool InputManager::initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        if (!m_keyboardSource)
        {
            return false;
        }

        if (!m_keyboardSource->initialize())
        {
            return false;
        }

        m_initialized = true;
        return true;
    }

    void InputManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_keyboardSource)
        {
            m_keyboardSource->shutdown();
        }

        m_events.clear();
        m_initialized = false;
    }

    void InputManager::poll()
    {
        if (!m_initialized || !m_keyboardSource)
        {
            return;
        }

        while (true)
        {
            std::optional<RawKeyEvent> raw = m_keyboardSource->pollRawKey();
            if (!raw.has_value())
            {
                break;
            }

            m_events.push_back(normalizeRawKey(raw.value()));
        }
    }

    bool InputManager::hasEvents() const
    {
        return !m_events.empty();
    }

    std::optional<KeyEvent> InputManager::popEvent()
    {
        if (m_events.empty())
        {
            return std::nullopt;
        }

        KeyEvent event = m_events.front();
        m_events.erase(m_events.begin());
        return event;
    }

    std::vector<KeyEvent> InputManager::drainEvents()
    {
        std::vector<KeyEvent> drained;
        drained.swap(m_events);
        return drained;
    }

    void InputManager::clear()
    {
        m_events.clear();
    }

    KeyEvent InputManager::normalizeRawKey(const RawKeyEvent& raw) const
    {
        KeyEvent event;
        event.code = mapVirtualKey(raw.virtualKey, raw.character);
        event.character = raw.character;
        event.modifiers = raw.modifiers;
        event.pressed = raw.pressed;
        event.repeatCount = raw.repeatCount == 0 ? 1 : raw.repeatCount;

        return event;
    }
}
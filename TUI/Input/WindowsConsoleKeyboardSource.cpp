#include "Input/WindowsConsoleKeyboardSource.h"

#define NOMINMAX
#include <windows.h>

namespace Input
{
    namespace
    {
        KeyModifiers readModifiers(DWORD controlKeyState)
        {
            KeyModifiers modifiers;
            modifiers.shift = (controlKeyState & SHIFT_PRESSED) != 0;
            modifiers.ctrl =
                (controlKeyState & LEFT_CTRL_PRESSED) != 0 ||
                (controlKeyState & RIGHT_CTRL_PRESSED) != 0;
            modifiers.alt =
                (controlKeyState & LEFT_ALT_PRESSED) != 0 ||
                (controlKeyState & RIGHT_ALT_PRESSED) != 0;

            return modifiers;
        }
    }

    WindowsConsoleKeyboardSource::~WindowsConsoleKeyboardSource()
    {
        shutdown();
    }

    bool WindowsConsoleKeyboardSource::initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
        if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
        {
            return false;
        }

        DWORD mode = 0;
        if (!GetConsoleMode(handle, &mode))
        {
            return false;
        }

        m_inputHandle = handle;
        m_originalMode = mode;

        DWORD rawMode = mode;

        // Keyboard polling should own only keyboard/window input for Phase 7.
        // Do not preserve mouse or VT input here; mouse support is not implemented yet,
        // and VT mouse/escape sequences can be misread as Escape -> Cancel -> Quit.
        rawMode &= ~ENABLE_LINE_INPUT;
        rawMode &= ~ENABLE_ECHO_INPUT;
        rawMode &= ~ENABLE_PROCESSED_INPUT;
        rawMode &= ~ENABLE_MOUSE_INPUT;
        rawMode &= ~ENABLE_VIRTUAL_TERMINAL_INPUT;

        rawMode |= ENABLE_WINDOW_INPUT;

        if (!SetConsoleMode(handle, rawMode))
        {
            m_inputHandle = nullptr;
            m_originalMode = 0;
            return false;
        }

        m_initialized = true;
        return true;
    }

    void WindowsConsoleKeyboardSource::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        HANDLE handle = static_cast<HANDLE>(m_inputHandle);
        if (handle != INVALID_HANDLE_VALUE && handle != nullptr)
        {
            SetConsoleMode(handle, m_originalMode);
        }

        m_inputHandle = nullptr;
        m_originalMode = 0;
        m_initialized = false;
    }

    std::optional<RawKeyEvent> WindowsConsoleKeyboardSource::pollRawKey()
    {
        if (!m_initialized)
        {
            return std::nullopt;
        }

        HANDLE handle = static_cast<HANDLE>(m_inputHandle);

        while (true)
        {
            DWORD eventCount = 0;
            if (!GetNumberOfConsoleInputEvents(handle, &eventCount) || eventCount == 0)
            {
                return std::nullopt;
            }

            INPUT_RECORD record{};
            DWORD recordsRead = 0;
            if (!ReadConsoleInputW(handle, &record, 1, &recordsRead) || recordsRead == 0)
            {
                return std::nullopt;
            }

            if (record.EventType != KEY_EVENT)
            {
                continue;
            }

            const KEY_EVENT_RECORD& key = record.Event.KeyEvent;

            RawKeyEvent raw;
            raw.virtualKey = static_cast<std::uint16_t>(key.wVirtualKeyCode);
            raw.character = static_cast<char32_t>(key.uChar.UnicodeChar);
            raw.modifiers = readModifiers(key.dwControlKeyState);
            raw.pressed = key.bKeyDown != FALSE;
            raw.repeatCount = static_cast<std::uint16_t>(key.wRepeatCount == 0 ? 1 : key.wRepeatCount);

            return raw;
        }
    }
}
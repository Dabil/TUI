#pragma once

#include "Input/RawKeyboardSource.h"

namespace Input
{
    class WindowsConsoleKeyboardSource final : public IRawKeyboardSource
    {
    public:
        WindowsConsoleKeyboardSource() = default;
        ~WindowsConsoleKeyboardSource() override;

        bool initialize() override;
        void shutdown() override;

        std::optional<RawKeyEvent> pollRawKey() override;

    private:
        void* m_inputHandle = nullptr;
        unsigned long m_originalMode = 0;
        bool m_initialized = false;
    };
}
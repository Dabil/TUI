#include "App/TerminalLauncher.h"

#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>

namespace
{
    constexpr wchar_t kLaunchedByWtFlag[] = L"--launched-by-wt";

    bool equalsIgnoreCase(const wchar_t* left, const wchar_t* right)
    {
        if (left == nullptr || right == nullptr)
        {
            return false;
        }

        return _wcsicmp(left, right) == 0;
    }

    bool hasArgument(int argc, wchar_t** argv, const wchar_t* value)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (equalsIgnoreCase(argv[i], value))
            {
                return true;
            }
        }

        return false;
    }

    bool hasWindowsTerminalSessionHint()
    {
        const DWORD required = GetEnvironmentVariableW(L"WT_SESSION", nullptr, 0);
        return required > 0;
    }

    std::wstring getModulePath()
    {
        std::wstring result;
        result.resize(32768);

        const DWORD length = GetModuleFileNameW(nullptr, &result[0], static_cast<DWORD>(result.size()));
        if (length == 0)
        {
            return std::wstring();
        }

        result.resize(length);
        return result;
    }

    std::wstring getCurrentDirectoryString()
    {
        const DWORD required = GetCurrentDirectoryW(0, nullptr);
        if (required == 0)
        {
            return std::wstring();
        }

        std::wstring result;
        result.resize(required);

        const DWORD length = GetCurrentDirectoryW(required, &result[0]);
        if (length == 0)
        {
            return std::wstring();
        }

        if (!result.empty() && result.back() == L'\0')
        {
            result.pop_back();
        }

        return result;
    }

    std::wstring quoteWindowsArgument(const std::wstring& value)
    {
        if (value.empty())
        {
            return L"\"\"";
        }

        bool needsQuotes = false;
        for (wchar_t ch : value)
        {
            if (ch == L' ' || ch == L'\t' || ch == L'"')
            {
                needsQuotes = true;
                break;
            }
        }

        if (!needsQuotes)
        {
            return value;
        }

        std::wstring result;
        result.push_back(L'"');

        int backslashCount = 0;

        for (wchar_t ch : value)
        {
            if (ch == L'\\')
            {
                ++backslashCount;
                continue;
            }

            if (ch == L'"')
            {
                result.append(backslashCount * 2 + 1, L'\\');
                result.push_back(L'"');
                backslashCount = 0;
                continue;
            }

            if (backslashCount > 0)
            {
                result.append(backslashCount, L'\\');
                backslashCount = 0;
            }

            result.push_back(ch);
        }

        if (backslashCount > 0)
        {
            result.append(backslashCount * 2, L'\\');
        }

        result.push_back(L'"');
        return result;
    }

    std::wstring buildWtCommandLine(int argc, wchar_t** argv)
    {
        const std::wstring exePath = getModulePath();
        if (exePath.empty())
        {
            return std::wstring();
        }

        const std::wstring currentDirectory = getCurrentDirectoryString();

        std::wstring commandLine;

        commandLine += L"wt.exe";
        commandLine += L" -w -1";
        commandLine += L" -M";
        commandLine += L" -d ";
        commandLine += quoteWindowsArgument(currentDirectory);
        commandLine += L" ";
        commandLine += quoteWindowsArgument(exePath);
        commandLine += L" ";
        commandLine += kLaunchedByWtFlag;

        for (int i = 1; i < argc; ++i)
        {
            if (equalsIgnoreCase(argv[i], kLaunchedByWtFlag))
            {
                continue;
            }

            commandLine += L" ";
            commandLine += quoteWindowsArgument(argv[i]);
        }

        return commandLine;
    }
}

bool TerminalLauncher::tryRelaunchInWindowsTerminal()
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr)
    {
        return false;
    }

    const bool alreadyRelaunched = hasArgument(argc, argv, kLaunchedByWtFlag);
    const bool alreadyInsideWindowsTerminal = hasWindowsTerminalSessionHint();

    if (alreadyRelaunched || alreadyInsideWindowsTerminal)
    {
        LocalFree(argv);
        return false;
    }

    const std::wstring commandLine = buildWtCommandLine(argc, argv);
    LocalFree(argv);

    if (commandLine.empty())
    {
        return false;
    }

    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};
    const BOOL created = CreateProcessW(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    if (!created)
    {
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

bool TerminalLauncher::shouldPreferTerminalRenderer()
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr)
    {
        return hasWindowsTerminalSessionHint();
    }

    const bool launchedByWt = hasArgument(argc, argv, kLaunchedByWtFlag);
    LocalFree(argv);

    return launchedByWt || hasWindowsTerminalSessionHint();
}
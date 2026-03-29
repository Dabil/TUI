#pragma once

#include <string>

enum class StartupHostPreference
{
    Auto = 0,
    Console,
    Terminal
};

enum class StartupRendererPreference
{
    Auto = 0,
    Console,
    Terminal
};

struct StartupConfig
{
    StartupHostPreference hostPreference = StartupHostPreference::Auto;
    StartupRendererPreference rendererPreference = StartupRendererPreference::Auto;

    bool configFileFound = false;
    std::wstring configFilePath;
};

class StartupConfigLoader
{
public:
    static StartupConfig loadFromStartupIni();

private:
    static std::wstring getStartupIniPath();
};
#pragma once

#include "Screens/Screen.h"
#include "Assets/AssetLibrary.h"


class Surface;

///////////////////////////////////////////////////////////////////////////////////////////
// S H O W C A S E   C O N T R O L L E R

class TimedShowcaseScreen : public Screen
{
public:
    ~TimedShowcaseScreen() override = default;

    void onEnter() override;
    void update(double deltaTime) override;

protected:
    double elapsedSeconds() const;

private:
    double m_elapsedSeconds = 0.0;
};

///////////////////////////////////////////////////////////////////////////////////////////
// C O N T R O L   D E C K

class ControlDeckScreen final : public TimedShowcaseScreen
{
public:
    ControlDeckScreen() = default;
    ~ControlDeckScreen() override = default;

    void draw(Surface& surface) override;
};

///////////////////////////////////////////////////////////////////////////////////////////
// R E T R O   T E R M I N A L

class RetroTerminalScreen final : public TimedShowcaseScreen
{
public:
    explicit RetroTerminalScreen(Assets::AssetLibrary& assetLibrary);
    ~RetroTerminalScreen() override = default;

    void onEnter() override;
    void onExit() override;
    void draw(Surface& surface) override;

private:

    Assets::AssetLibrary& m_assetLibrary;

    std::string m_retroTerminalAssetKey = "xp.retro_terminal_1";
    TextObject m_retroTerminalObject;
    bool m_retroTerminalLoadAttempted = false;
    std::string m_retroTerminalLoadError;
};

///////////////////////////////////////////////////////////////////////////////////////////
// N E O N   D I A L O G

class NeonDialogScreen final : public TimedShowcaseScreen
{
public:
    NeonDialogScreen() = default;
    ~NeonDialogScreen() override = default;

    void draw(Surface& surface) override;
};

class OpsWallScreen final : public TimedShowcaseScreen
{
public:
    OpsWallScreen() = default;
    ~OpsWallScreen() override = default;

    void draw(Surface& surface) override;
};


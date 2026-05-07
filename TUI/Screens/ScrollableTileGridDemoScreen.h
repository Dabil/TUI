#pragma once

#include <string>

#include "Input/Command.h"
#include "Input/Event.h"
#include "Rendering/Styles/Style.h"
#include "Screens/Screen.h"
#include "UI/Panels/Panel.h"
#include "UI/Panels/StatusBar.h"
#include "UI/Widgets/TileGridView.h"

class Surface;

class ScrollableTileGridDemoScreen : public Screen
{
public:
    ScrollableTileGridDemoScreen();
    ~ScrollableTileGridDemoScreen() override = default;

    void onEnter() override;
    bool handleEvent(const Input::Event& event) override;
    void draw(Surface& surface) override;

private:
    void configureControls();
    void buildDemoGrid();

    void updateLayout(Surface& surface);
    void updateStatusBar();

    bool handleCommand(const Input::Command& command);

    void drawHeader(Surface& surface) const;
    void drawTooSmall(Surface& surface) const;

    static TileGrid::Cell makeCell(char32_t glyph, const Style& style);

private:
    Panel m_headerPanel;
    TileGridView m_gridView;
    StatusBar m_statusBar;

    std::string m_lastAction;
};
#pragma once

#include <string>
#include <vector>

#include "Input/Command.h"
#include "Input/Event.h"
#include "Screens/Screen.h"
#include "UI/Base/FocusManager.h"
#include "UI/Base/WindowManager.h"
#include "UI/Menus/MenuController.h"
#include "UI/Menus/MenuModel.h"
#include "UI/Menus/MenuView.h"
#include "UI/Panels/Panel.h"
#include "UI/Panels/PopupWindow.h"
#include "UI/Panels/StatusBar.h"

class Surface;

class MenuDemoScreen : public Screen
{
public:
    MenuDemoScreen();
    ~MenuDemoScreen() override = default;

    void onEnter() override;
    bool handleEvent(const Input::Event& event) override;
    void update(const Animation::TickEvent& event) override;
    void draw(Surface& surface) override;

private:
    void buildMenu();
    void configureControls();
    void updateStatusBar();
    void updateLayout(Surface& surface);

    bool handleCommand(const Input::Command& command);
    void handleMenuResult(const UI::Menus::MenuControllerResult& result);

    void confirmSelection(const std::string& commandId);
    void openPopup(std::string title, std::vector<std::string> bodyLines);
    void closePopup();

    void drawTooSmall(Surface& surface) const;
    void drawBodyText(Surface& surface) const;
    void drawPopupContent(Surface& surface) const;

private:
    UI::Menus::MenuModel m_menuModel;
    UI::Menus::MenuView m_menuView;
    UI::Menus::MenuController m_menuController;

    FocusManager m_focusManager;
    WindowManager m_windowManager;

    Panel m_headerPanel;
    Panel m_menuPanel;
    Panel m_detailPanel;
    PopupWindow m_popup;
    StatusBar m_statusBar;

    std::vector<std::string> m_popupLines;
    std::string m_lastAction;
    bool m_soundEnabled = true;
    bool m_compactMode = false;
};
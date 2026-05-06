#include "Screens/MenuDemoScreen.h"

#include <algorithm>
#include <utility>

#include "Core/Rect.h"
#include "Input/Command.h"
#include "Input/Event.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Styles/UIThemes.h"
#include "Rendering/Surface.h"
#include "Utilities/Text/TextClip.h"

namespace
{
    using Composition::PageComposer;
    using UI::Menus::MenuControllerAction;
    using UI::Menus::MenuNavigationMode;
    using UI::Menus::MenuOrientation;

    constexpr int MinimumWidth = 86;
    constexpr int MinimumHeight = 28;

    const Style ScreenStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style HeaderStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Blue))
        + style::Bold;

    const Style PanelStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style BorderStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Black))
        + style::Bold;

    const Style AccentStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black))
        + style::Bold;

    const Style MutedStyle =
        style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black))
        + style::Dim;

    const Style DisabledStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    void writeClipped(ScreenBuffer& buffer, int x, int y, int width, const std::string& text, const Style& style)
    {
        if (width <= 0)
        {
            return;
        }

        buffer.writeString(x, y, TextClip::clipUtf8Text(text, width), style);
    }
}

MenuDemoScreen::MenuDemoScreen()
    : m_menuController(m_menuModel, m_menuView)
    , m_popup(Rect{}, "Help / About", true)
{
    buildMenu();
    configureControls();

    m_windowManager.addWindow(m_popup);

    m_focusManager.addFocusTarget(m_menuView);
    m_focusManager.addFocusTarget(m_popup);
}

void MenuDemoScreen::onEnter()
{
    m_menuController.selectFirstEnabled();
    m_focusManager.setFocus(m_menuView);
    m_popup.close();

    m_lastAction = "Use Up/Down to choose a menu item, then press Enter.";
    updateStatusBar();
}

bool MenuDemoScreen::handleEvent(const Input::Event& event)
{
    if (const Input::CommandEvent* commandEvent = event.asCommand())
    {
        return handleCommand(commandEvent->command);
    }

    return false;
}

void MenuDemoScreen::update(double deltaTime)
{
    m_windowManager.update(deltaTime);
}

void MenuDemoScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    if (buffer.getWidth() < MinimumWidth || buffer.getHeight() < MinimumHeight)
    {
        drawTooSmall(surface);
        return;
    }

    buffer.clear(ScreenStyle);

    updateLayout(surface);

    m_headerPanel.draw(surface);
    m_menuPanel.draw(surface);
    m_detailPanel.draw(surface);

    m_menuView.setHasFocus(m_focusManager.hasFocus(m_menuView));
    m_menuView.draw(surface);

    drawBodyText(surface);

    m_windowManager.draw(surface);
    drawPopupContent(surface);

    m_statusBar.draw(surface);
}

void MenuDemoScreen::buildMenu()
{
    m_menuModel.clear();

    m_menuModel.addItem("Start authored page flow", "start", "Runs a fake authored-page action and updates the details panel.");
    m_menuModel.addItem("Settings", "settings", "Opens a lightweight settings popup.");
    m_menuModel.addItem("Toggle sound hints", "toggle_sound", "Toggles a checked menu item without persistence.");
    m_menuModel.addItem("Toggle compact layout", "toggle_compact", "Toggles another checked menu item.");
    m_menuModel.addItem("Continue saved session", "continue", "Disabled sample item to prove disabled skipping.");
    m_menuModel.addItem("Help / About", "help", "Shows a modal popup using PopupWindow and WindowManager.");

    m_menuModel.setEnabled(4, false);
    m_menuModel.setChecked(2, m_soundEnabled);
    m_menuModel.setChecked(3, m_compactMode);
}

void MenuDemoScreen::configureControls()
{
    m_headerPanel.setTitle("Interactive Authored Page");
    m_headerPanel.setBackgroundStyle(HeaderStyle);
    m_headerPanel.setBorderStyle(HeaderStyle);
    m_headerPanel.setTitleStyle(HeaderStyle);

    m_menuPanel.setTitle("Title Menu");
    m_menuPanel.setBackgroundStyle(PanelStyle);
    m_menuPanel.setBorderStyle(BorderStyle);
    m_menuPanel.setTitleStyle(AccentStyle);

    m_detailPanel.setTitle("Context");
    m_detailPanel.setBackgroundStyle(PanelStyle);
    m_detailPanel.setBorderStyle(BorderStyle);
    m_detailPanel.setTitleStyle(AccentStyle);

    m_popup.setBackgroundStyle(PanelStyle);
    m_popup.setBorderStyle(AccentStyle);
    m_popup.setTitleStyle(AccentStyle);
    m_popup.close();

    m_menuView.setModel(m_menuModel);
    m_menuView.setOrientation(MenuOrientation::Vertical);
    m_menuView.setItemSpacing(1);
    m_menuView.setShowCheckedIndicator(true);
    m_menuView.setItemStyle(UIThemes::Label);
    m_menuView.setSelectedStyle(UIThemes::Selection);
    m_menuView.setFocusedStyle(UIThemes::Focused);
    m_menuView.setDisabledStyle(DisabledStyle);

    m_menuController.setOrientation(MenuOrientation::Vertical);
    m_menuController.setNavigationMode(MenuNavigationMode::Wrap);
    m_menuController.setOnSelectionChanged(
        [this](std::size_t, const UI::Menus::MenuItem& item)
        {
            m_lastAction = item.hasDescription()
                ? item.description()
                : "Ready.";
            updateStatusBar();
        });

    m_statusBar.setInstructions("Interactive Menu demo");
    m_statusBar.setCommandHints({ "Up/Down: select", "Enter/Space: confirm", "Esc: cancel/close", "F1: help" });
}

void MenuDemoScreen::updateStatusBar()
{
    if (m_popup.isOpen())
    {
        m_statusBar.setInstructions("Popup open");
        m_statusBar.setCommandHints({ "Enter/Esc: close popup", "Tab: focus" });
        return;
    }

    m_statusBar.setInstructions("Menu focused");
    m_statusBar.setCommandHints({ "Up/Down: select", "Enter/Space: confirm", "Esc: cancel", "F1: help" });
}

void MenuDemoScreen::updateLayout(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    PageComposer page(buffer);
    page.clearRegions();
    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Safe", "Screen", 2, 1, 2, 1);

    const Rect safe = page.resolveRegion("Safe");
    const auto [header, afterHeader] = page.splitTop(safe, 5);
    const auto [status, body] = page.splitBottom(afterHeader, 3);
    const auto [menuColumn, detailColumn] = page.splitLeft(body, 34);

    const Rect menuPanelRect{
        Point{ menuColumn.position.x, menuColumn.position.y + 1 },
        Size{ std::max(0, menuColumn.size.width - 1), std::max(0, menuColumn.size.height - 2) }
    };

    const Rect detailPanelRect{
        Point{ detailColumn.position.x + 1, detailColumn.position.y + 1 },
        Size{ std::max(0, detailColumn.size.width - 1), std::max(0, detailColumn.size.height - 2) }
    };

    m_headerPanel.setBounds(header);
    m_menuPanel.setBounds(menuPanelRect);
    m_detailPanel.setBounds(detailPanelRect);
    m_statusBar.setBounds(status);

    const Rect menuContent = m_menuPanel.contentBounds();
    m_menuView.setBounds(Rect{
        Point{ menuContent.position.x + 1, menuContent.position.y + 1 },
        Size{ std::max(0, menuContent.size.width - 2), std::max(0, menuContent.size.height - 2) }
        });

    const int popupWidth = std::min(64, std::max(40, buffer.getWidth() - 12));
    const int popupHeight = 12;
    m_popup.setBounds(Rect{
        Point{ (buffer.getWidth() - popupWidth) / 2, (buffer.getHeight() - popupHeight) / 2 },
        Size{ popupWidth, popupHeight }
        });
}

bool MenuDemoScreen::handleCommand(const Input::Command& command)
{
    using Input::CommandCode;

    if (m_popup.isOpen())
    {
        switch (command.code)
        {
        case CommandCode::Confirm:
        case CommandCode::Cancel:
        case CommandCode::Close:
            closePopup();
            return true;

        case CommandCode::NextFocus:
            m_focusManager.focusNext();
            return true;

        case CommandCode::PreviousFocus:
            m_focusManager.focusPrevious();
            return true;

        default:
            return true;
        }
    }

    UI::Menus::MenuControllerResult result;

    switch (command.code)
    {
    case CommandCode::MoveUp:
        result = m_menuController.up();
        handleMenuResult(result);
        return result.handled();

    case CommandCode::MoveDown:
        result = m_menuController.down();
        handleMenuResult(result);
        return result.handled();

    case CommandCode::MoveHome:
        result = m_menuController.selectFirstEnabled();
        handleMenuResult(result);
        return result.handled();

    case CommandCode::MoveEnd:
        result = m_menuController.selectLastEnabled();
        handleMenuResult(result);
        return result.handled();

    case CommandCode::Confirm:
        result = m_menuController.confirm();
        handleMenuResult(result);
        return result.handled();

    case CommandCode::Cancel:
        m_lastAction = "Cancelled the current authored-page action.";
        updateStatusBar();
        return true;

    case CommandCode::OpenHelp:
        confirmSelection("help");
        return true;

    case CommandCode::NextFocus:
        m_focusManager.focusNext();
        updateStatusBar();
        return true;

    case CommandCode::PreviousFocus:
        m_focusManager.focusPrevious();
        updateStatusBar();
        return true;

    default:
        return false;
    }
}

void MenuDemoScreen::handleMenuResult(const UI::Menus::MenuControllerResult& result)
{
    if (!result.handled())
    {
        return;
    }

    if (result.confirmed())
    {
        confirmSelection(result.commandId);
    }
}

void MenuDemoScreen::confirmSelection(const std::string& commandId)
{
    if (commandId == "start")
    {
        m_lastAction = "Started the authored-page flow. This is where a real page transition or action would run.";
    }
    else if (commandId == "settings")
    {
        openPopup(
            "Settings",
            {
                "Settings are intentionally lightweight in this demo.",
                "",
                std::string("Sound hints: ") + (m_soundEnabled ? "enabled" : "disabled"),
                std::string("Compact layout: ") + (m_compactMode ? "enabled" : "disabled"),
                "",
                "Use the toggle menu items to change these values."
            });
    }
    else if (commandId == "toggle_sound")
    {
        m_soundEnabled = !m_soundEnabled;
        m_menuModel.setChecked(2, m_soundEnabled);
        m_lastAction = std::string("Sound hints are now ") + (m_soundEnabled ? "enabled." : "disabled.");
    }
    else if (commandId == "toggle_compact")
    {
        m_compactMode = !m_compactMode;
        m_menuModel.setChecked(3, m_compactMode);
        m_lastAction = std::string("Compact layout is now ") + (m_compactMode ? "enabled." : "disabled.");
    }
    else if (commandId == "help")
    {
        openPopup(
            "Help / About",
            {
                "This is the first interactive authored page.",
                "",
                "It combines FocusManager, MenuModel, MenuView,",
                "MenuController, StatusBar, PopupWindow, and WindowManager.",
                "",
                "Disabled menu items are skipped during navigation."
            });
    }

    updateStatusBar();
}

void MenuDemoScreen::openPopup(std::string title, std::vector<std::string> bodyLines)
{
    m_popup.setTitle(std::move(title));
    m_popupLines = std::move(bodyLines);
    m_popup.open();

    m_windowManager.show(m_popup);
    m_windowManager.bringToFront(m_popup);

    m_focusManager.setFocus(m_popup);
    updateStatusBar();
}

void MenuDemoScreen::closePopup()
{
    m_popup.close();
    m_popupLines.clear();

    m_focusManager.setFocus(m_menuView);
    updateStatusBar();
}

void MenuDemoScreen::drawTooSmall(Surface& surface) const
{
    ScreenBuffer& buffer = surface.buffer();
    buffer.clear(ScreenStyle);

    buffer.writeString(2, 2, "InteractiveAuthoredPageDemoScreen needs a larger terminal.", AccentStyle);
    buffer.writeString(2, 4, "Recommended minimum: 86 x 28", MutedStyle);
}

void MenuDemoScreen::drawBodyText(Surface& surface) const
{
    ScreenBuffer& buffer = surface.buffer();

    const Rect header = m_headerPanel.contentBounds();
    writeClipped(buffer, header.position.x + 1, header.position.y + 1, header.size.width - 2,
        "A real menu-driven authored page using the new interaction primitives.", HeaderStyle);

    const Rect detail = m_detailPanel.contentBounds();
    int y = detail.position.y + 1;
    const int x = detail.position.x + 2;
    const int width = detail.size.width - 4;

    writeClipped(buffer, x, y++, width, "Current selection / action:", AccentStyle);
    y++;
    writeClipped(buffer, x, y++, width, m_lastAction, PanelStyle);

    y += 2;
    writeClipped(buffer, x, y++, width, "Live authored state:", AccentStyle);
    writeClipped(buffer, x, y++, width, std::string("- Sound hints: ") + (m_soundEnabled ? "on" : "off"), PanelStyle);
    writeClipped(buffer, x, y++, width, std::string("- Compact layout: ") + (m_compactMode ? "on" : "off"), PanelStyle);
    writeClipped(buffer, x, y++, width, "- Disabled item: Continue saved session", DisabledStyle);

    y += 2;
    writeClipped(buffer, x, y++, width, "This screen intentionally does not add persistence or a new lifecycle.", MutedStyle);
    writeClipped(buffer, x, y++, width, "It is a focused integration demo for authored interactive pages.", MutedStyle);
}

void MenuDemoScreen::drawPopupContent(Surface& surface) const
{
    if (!m_popup.isOpen())
    {
        return;
    }

    ScreenBuffer& buffer = surface.buffer();
    const Rect content = m_popup.contentBounds();

    int y = content.position.y + 1;
    const int x = content.position.x + 2;
    const int width = content.size.width - 4;

    for (const std::string& line : m_popupLines)
    {
        if (y >= content.position.y + content.size.height - 2)
        {
            break;
        }

        writeClipped(buffer, x, y, width, line, line.empty() ? PanelStyle : PanelStyle);
        ++y;
    }

    const int hintY = content.position.y + content.size.height - 1;
    writeClipped(buffer, x, hintY, width, "Press Enter or Esc to close.", AccentStyle);
}
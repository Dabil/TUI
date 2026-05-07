#include "Screens/WidgetDemoScreen.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "Core/Rect.h"
#include "Input/Command.h"
#include "Input/Event.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Styles/UIThemes.h"
#include "Rendering/Surface.h"

namespace
{
    using Composition::PageComposer;

    constexpr int MinimumWidth = 88;
    constexpr int MinimumHeight = 30;

    const Style ScreenStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style PanelStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style HeaderStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Blue))
        + style::Bold;

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
}

WidgetDemoScreen::WidgetDemoScreen()
{
    buildWidgets();
    configureStyles();
}

void WidgetDemoScreen::onEnter()
{
    m_root.focusFirstChild();
    m_lastAction = "Widget demo ready. Use Tab to move focus between controls.";
    syncSelectionMessage();
    updateStatusBar();
}

bool WidgetDemoScreen::handleEvent(const Input::Event& event)
{
    if (const Input::CommandEvent* commandEvent = event.asCommand())
    {
        return handleCommand(commandEvent->command);
    }

    return m_root.handleEvent(event);
}

void WidgetDemoScreen::update(double deltaTime)
{
    m_root.update(deltaTime);
}

void WidgetDemoScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    if (buffer.getWidth() < MinimumWidth || buffer.getHeight() < MinimumHeight)
    {
        drawTooSmall(surface);
        return;
    }

    buffer.clear(ScreenStyle);

    updateLayout(surface);
    syncSelectionMessage();

    m_root.draw(surface);
    m_statusBar.draw(surface);
}

void WidgetDemoScreen::buildWidgets()
{
    auto headerPanel = std::make_unique<Panel>();
    m_headerPanel = headerPanel.get();
    m_root.addChild(std::move(headerPanel));

    auto titleLabel = std::make_unique<Label>("Phase 11 Widget Demo");
    m_titleLabel = titleLabel.get();
    m_root.addChild(std::move(titleLabel));

    auto controlPanel = std::make_unique<Panel>();
    m_controlPanel = controlPanel.get();
    m_root.addChild(std::move(controlPanel));

    auto listBox = std::make_unique<ListBox>();
    listBox->addItem("Dashboard summary");
    listBox->addItem("Project activity");
    listBox->addItem("Disabled report sample", false);
    listBox->addItem("System messages");
    listBox->addItem("Release checklist");
    m_listBox = listBox.get();
    m_root.addChild(std::move(listBox));

    auto applyButton = std::make_unique<Button>("Apply Selection");
    applyButton->setCommandId("apply");
    applyButton->setActivationCallback(
        [this](const ButtonActivationResult& result)
        {
            handleButtonActivation(result);
        });
    m_applyButton = applyButton.get();
    m_root.addChild(std::move(applyButton));

    auto resetButton = std::make_unique<Button>("Reset List");
    resetButton->setCommandId("reset");
    resetButton->setActivationCallback(
        [this](const ButtonActivationResult& result)
        {
            handleButtonActivation(result);
        });
    m_resetButton = resetButton.get();
    m_root.addChild(std::move(resetButton));

    auto disabledButton = std::make_unique<Button>("Disabled Action");
    disabledButton->setCommandId("disabled");
    disabledButton->setEnabled(false);
    disabledButton->disable();
    m_disabledButton = disabledButton.get();
    m_root.addChild(std::move(disabledButton));

    auto stateLabel = std::make_unique<Label>();
    m_stateLabel = stateLabel.get();
    m_root.addChild(std::move(stateLabel));

    auto textPanel = std::make_unique<Panel>();
    m_textPanel = textPanel.get();
    m_root.addChild(std::move(textPanel));

    auto textView = std::make_unique<TextView>("Read-only TextView");
    textView->setLines({
        "This TextView is owned by the same ContainerWidget as the Label, Button, and ListBox controls.",
        "",
        "What this screen proves:",
        "- ContainerWidget owns child widgets with std::unique_ptr.",
        "- Children draw in stable insertion order.",
        "- Tab / Shift+Tab traverses focusable children.",
        "- Button activation is routed through reusable Button callbacks.",
        "- ListBox selection skips disabled items.",
        "- Focused, disabled, and selected styles are visible.",
        "",
        "Try focusing this TextView and using Up/Down/PageUp/PageDown.",
        "The screen does not replace the existing Screen lifecycle."
        });
    m_textView = textView.get();
    m_root.addChild(std::move(textView));
}

void WidgetDemoScreen::configureStyles()
{
    m_headerPanel->setTitle("Reusable Widget Framework");
    m_headerPanel->setBackgroundStyle(HeaderStyle);
    m_headerPanel->setBorderStyle(HeaderStyle);
    m_headerPanel->setTitleStyle(HeaderStyle);

    m_controlPanel->setTitle("Controls");
    m_controlPanel->setBackgroundStyle(PanelStyle);
    m_controlPanel->setBorderStyle(BorderStyle);
    m_controlPanel->setTitleStyle(AccentStyle);

    m_textPanel->setTitle("Read-only Content");
    m_textPanel->setBackgroundStyle(PanelStyle);
    m_textPanel->setBorderStyle(BorderStyle);
    m_textPanel->setTitleStyle(AccentStyle);

    m_titleLabel->setTextStyle(HeaderStyle);
    m_stateLabel->setTextStyle(AccentStyle);

    m_statusBar.setInstructions("Widget demo");
    m_statusBar.setCommandHints({
        "Tab: next focus",
        "Shift+Tab: previous focus",
        "Up/Down: list/text",
        "Enter: button/list action",
        "Esc: cancel"
        });
}

void WidgetDemoScreen::updateLayout(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    PageComposer page(buffer);
    page.clearRegions();
    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Safe", "Screen", 2, 1, 2, 1);

    const Rect safe = page.resolveRegion("Safe");
    const auto [header, afterHeader] = page.splitTop(safe, 5);
    const auto [status, body] = page.splitBottom(afterHeader, 3);
    const auto [controls, textArea] = page.splitLeft(body, 36);

    m_root.setBounds(safe);
    m_headerPanel->setBounds(header);
    m_statusBar.setBounds(status);

    const Rect headerContent = m_headerPanel->contentBounds();
    m_titleLabel->setBounds(Rect{
        Point{ headerContent.position.x + 2, headerContent.position.y + 1 },
        Size{ std::max(0, headerContent.size.width - 4), 1 }
        });

    m_controlPanel->setBounds(Rect{
        Point{ controls.position.x, controls.position.y + 1 },
        Size{ std::max(0, controls.size.width - 1), std::max(0, controls.size.height - 2) }
        });

    const Rect controlContent = m_controlPanel->contentBounds();
    m_listBox->setBounds(Rect{
        Point{ controlContent.position.x + 2, controlContent.position.y + 1 },
        Size{ std::max(0, controlContent.size.width - 4), 7 }
        });

    const int buttonY = controlContent.position.y + 10;
    const int buttonWidth = std::max(10, (controlContent.size.width - 6) / 2);

    m_applyButton->setBounds(Rect{
        Point{ controlContent.position.x + 2, buttonY },
        Size{ buttonWidth, 3 }
        });

    m_resetButton->setBounds(Rect{
        Point{ controlContent.position.x + 4 + buttonWidth, buttonY },
        Size{ buttonWidth, 3 }
        });

    m_disabledButton->setBounds(Rect{
        Point{ controlContent.position.x + 2, buttonY + 4 },
        Size{ std::max(0, controlContent.size.width - 4), 3 }
        });

    m_stateLabel->setBounds(Rect{
        Point{ controlContent.position.x + 2, buttonY + 8 },
        Size{ std::max(0, controlContent.size.width - 4), 1 }
        });

    m_textPanel->setBounds(Rect{
        Point{ textArea.position.x + 1, textArea.position.y + 1 },
        Size{ std::max(0, textArea.size.width - 1), std::max(0, textArea.size.height - 2) }
        });

    const Rect textContent = m_textPanel->contentBounds();
    m_textView->setBounds(Rect{
        Point{ textContent.position.x + 1, textContent.position.y + 1 },
        Size{ std::max(0, textContent.size.width - 2), std::max(0, textContent.size.height - 2) }
        });
}

void WidgetDemoScreen::updateStatusBar()
{
    m_statusBar.setInstructions(m_lastAction);
}

bool WidgetDemoScreen::handleCommand(const Input::Command& command)
{
    if (command.code == Input::CommandCode::Confirm && m_root.focusedChild() == m_listBox)
    {
        activateSelectedListItem();
        return true;
    }

    if (command.code == Input::CommandCode::Cancel)
    {
        m_lastAction = "Cancelled widget demo action.";
        updateStatusBar();
        return true;
    }

    const bool handled = m_root.handleEvent(Input::Event::command(command));

    if (handled)
    {
        syncSelectionMessage();
        updateStatusBar();
    }

    return handled;
}

void WidgetDemoScreen::activateSelectedListItem()
{
    const std::string* selectedText = m_listBox->selectedText();

    if (selectedText == nullptr)
    {
        m_lastAction = "No enabled list item is selected.";
    }
    else
    {
        m_lastAction = "Confirmed list item: " + *selectedText;
    }

    updateStatusBar();
}

void WidgetDemoScreen::handleButtonActivation(const ButtonActivationResult& result)
{
    if (!result.activated)
    {
        return;
    }

    if (result.commandId == "apply")
    {
        activateSelectedListItem();
    }
    else if (result.commandId == "reset")
    {
        m_listBox->moveToFirst();
        m_lastAction = "ListBox reset to the first enabled item.";
        syncSelectionMessage();
        updateStatusBar();
    }
}

void WidgetDemoScreen::syncSelectionMessage()
{
    if (!m_listBox)
    {
        return;
    }

    const std::optional<std::size_t> selectedIndex = m_listBox->selectedIndex();
    if (selectedIndex == m_lastObservedSelection)
    {
        return;
    }

    m_lastObservedSelection = selectedIndex;

    const std::string* selectedText = m_listBox->selectedText();
    if (selectedText == nullptr)
    {
        m_stateLabel->setText("Selection: <none>");
        return;
    }

    m_stateLabel->setText("Selection: " + *selectedText);
}

void WidgetDemoScreen::drawTooSmall(Surface& surface) const
{
    ScreenBuffer& buffer = surface.buffer();
    buffer.clear(ScreenStyle);

    buffer.writeString(2, 2, "WidgetDemoScreen needs a larger terminal.", AccentStyle);
    buffer.writeString(2, 4, "Recommended minimum: 88 x 30", MutedStyle);
}
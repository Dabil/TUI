#include "Screens/WidgetDemoScreen.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include "Core/Rect.h"
#include "Input/Command.h"
#include "Input/Event.h"
#include "Input/MouseEvent.h"
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
    constexpr int DiagnosticsPanelHeight = 5;

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
    m_lastMouseSummary = "Position: x=- y=- | Button: none | Action: none";
    m_lastTargetSummary = "Target: <none>";
    m_lastFocusSummary = "Focus: <none>";
    m_lastResultSummary = "Result: waiting for mouse input";
    m_lastMouseAction = "Mouse diagnostics ready. Move, click, or wheel over the demo.";
    syncSelectionMessage();
    updateStatusBar();
}

bool WidgetDemoScreen::handleEvent(const Input::Event& event)
{
    if (const Input::CommandEvent* commandEvent = event.asCommand())
    {
        return handleCommand(commandEvent->command);
    }

    if (const Input::MouseEvent* mouseEvent = event.asMouse())
    {
        return handleMouseEvent(*mouseEvent);
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
    drawMouseDiagnosticsPanel(surface);
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
        "- Mouse position, target, focus, and result are visible in the diagnostics panel.",
        "- Mouse diagnostics do not take over the footer/status bar.",
        "",
        "Try focusing this TextView and using Up/Down/PageUp/PageDown.",
        "Try moving the mouse over the screen and watching the diagnostics panel.",
        "Try using the mouse wheel over this TextView.",
        "",
        "Additional scroll content follows so wheel behavior is easy to validate.",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "And we are at the bottom now. Congrats!"
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
    const auto [status, aboveStatus] = page.splitBottom(afterHeader, 3);
    const auto [diagnostics, body] = page.splitBottom(aboveStatus, DiagnosticsPanelHeight);
    const auto [controls, textArea] = page.splitLeft(body, 36);

    m_root.setBounds(safe);
    m_headerPanel->setBounds(header);
    m_statusBar.setBounds(status);
    m_diagnosticsBounds = diagnostics;

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

bool WidgetDemoScreen::handleMouseEvent(const Input::MouseEvent& mouseEvent)
{
    Widget* targetBeforeDispatch = m_root.hitTest(mouseEvent.position);
    const std::optional<std::size_t> selectionBefore = m_listBox
        ? m_listBox->selectedIndex()
        : std::nullopt;
    const Widget* focusedBefore = m_root.focusedChild();

    const bool handled = m_root.handleEvent(Input::Event::mouse(mouseEvent));

    const Widget* focusedAfter = m_root.focusedChild();
    const std::optional<std::size_t> selectionAfter = m_listBox
        ? m_listBox->selectedIndex()
        : std::nullopt;

    m_lastMouseSummary = describeMouseEvent(mouseEvent);
    m_lastTargetSummary = "Target: " + describeWidget(targetBeforeDispatch);
    m_lastFocusSummary = "Focus: " + describeWidget(focusedAfter);
    m_lastResultSummary = "Result: " + std::string(handled ? "Handled" : "Not handled");

    if (mouseEvent.isMove())
    {
        m_lastMouseAction = targetBeforeDispatch
            ? "Passive mouse movement tracked over " + describeWidget(targetBeforeDispatch) + "."
            : "Passive mouse movement tracked outside an enabled widget.";
    }
    else if (mouseEvent.isWheel())
    {
        m_lastMouseAction = handled
            ? "Mouse wheel event scrolled or was accepted by " + describeWidget(targetBeforeDispatch) + "."
            : "Mouse wheel event received, but no widget handled it.";
    }
    else if ((mouseEvent.isPress() || mouseEvent.isClick()) && focusedBefore != focusedAfter)
    {
        m_lastMouseAction = "Click-to-focus moved focus to " + describeWidget(focusedAfter) + ".";
    }
    else if (selectionBefore != selectionAfter)
    {
        const std::string* selectedText = m_listBox ? m_listBox->selectedText() : nullptr;
        m_lastMouseAction = selectedText
            ? "ListBox mouse selection changed to: " + *selectedText
            : "ListBox mouse selection changed.";
    }
    else if (handled)
    {
        m_lastMouseAction = "Mouse event handled by " + describeWidget(targetBeforeDispatch) + ".";
    }
    else
    {
        m_lastMouseAction = "Mouse event received outside an enabled widget.";
    }

    syncSelectionMessage();
    updateSelectionLabel();

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
    updateSelectionLabel();
}

void WidgetDemoScreen::updateSelectionLabel()
{
    if (!m_stateLabel || !m_listBox)
    {
        return;
    }

    const std::string* selectedText = m_listBox->selectedText();
    const std::string selectionText = selectedText
        ? "Selection: " + *selectedText
        : "Selection: <none>";

    m_stateLabel->setText(selectionText);
}

void WidgetDemoScreen::drawMouseDiagnosticsPanel(Surface& surface) const
{
    ScreenBuffer& buffer = surface.buffer();

    if (m_diagnosticsBounds.size.width <= 0 || m_diagnosticsBounds.size.height <= 0)
    {
        return;
    }

    buffer.fillRect(m_diagnosticsBounds, U' ', PanelStyle);
    buffer.drawFrame(
        m_diagnosticsBounds,
        BorderStyle,
        U'┌',
        U'┐',
        U'└',
        U'┘',
        U'─',
        U'│');

    if (m_diagnosticsBounds.size.width > 4)
    {
        buffer.writeString(
            m_diagnosticsBounds.position.x + 2,
            m_diagnosticsBounds.position.y,
            fitToWidth(" Mouse Diagnostics ", m_diagnosticsBounds.size.width - 4),
            AccentStyle);
    }

    const int contentX = m_diagnosticsBounds.position.x + 2;
    const int contentWidth = std::max(0, m_diagnosticsBounds.size.width - 4);

    if (contentWidth <= 0)
    {
        return;
    }

    const std::string line1 = m_lastMouseSummary;
    const std::string line2 = m_lastTargetSummary + " | " + m_lastFocusSummary + " | " + m_lastResultSummary;
    const std::string line3 = "Last mouse action: " + m_lastMouseAction;

    writePaddedLine(surface, contentX, m_diagnosticsBounds.position.y + 1, contentWidth, line1, PanelStyle);
    writePaddedLine(surface, contentX, m_diagnosticsBounds.position.y + 2, contentWidth, line2, PanelStyle);
    writePaddedLine(surface, contentX, m_diagnosticsBounds.position.y + 3, contentWidth, line3, PanelStyle);
}

std::string WidgetDemoScreen::fitToWidth(const std::string& text, int width) const
{
    if (width <= 0)
    {
        return std::string();
    }

    const std::size_t targetWidth = static_cast<std::size_t>(width);
    if (text.size() <= targetWidth)
    {
        return text;
    }

    if (targetWidth <= 3)
    {
        return text.substr(0, targetWidth);
    }

    return text.substr(0, targetWidth - 3) + "...";
}

void WidgetDemoScreen::writePaddedLine(
    Surface& surface,
    int x,
    int y,
    int width,
    const std::string& text,
    const Style& style) const
{
    if (width <= 0)
    {
        return;
    }

    std::string line = fitToWidth(text, width);
    if (line.size() < static_cast<std::size_t>(width))
    {
        line.append(static_cast<std::size_t>(width) - line.size(), ' ');
    }

    surface.buffer().writeString(x, y, line, style);
}

std::string WidgetDemoScreen::describeMouseEvent(const Input::MouseEvent& mouseEvent) const
{
    std::ostringstream stream;
    stream << "Position: x=" << mouseEvent.position.x
        << " y=" << mouseEvent.position.y
        << " | Button: " << describeMouseButton(mouseEvent.button)
        << " | Action: " << describeMouseAction(mouseEvent.action);

    if (mouseEvent.isWheel())
    {
        stream << " delta=" << mouseEvent.wheelDelta;
    }

    if (mouseEvent.clickCount > 0)
    {
        stream << " clicks=" << mouseEvent.clickCount;
    }

    return stream.str();
}

std::string WidgetDemoScreen::describeMouseButton(Input::MouseButton button) const
{
    switch (button)
    {
    case Input::MouseButton::None:
        return "None";
    case Input::MouseButton::Left:
        return "Left";
    case Input::MouseButton::Right:
        return "Right";
    case Input::MouseButton::Middle:
        return "Middle";
    case Input::MouseButton::WheelUp:
        return "WheelUp";
    case Input::MouseButton::WheelDown:
        return "WheelDown";
    default:
        return "Unknown";
    }
}

std::string WidgetDemoScreen::describeMouseAction(Input::MouseAction action) const
{
    switch (action)
    {
    case Input::MouseAction::Moved:
        return "Moved";
    case Input::MouseAction::Pressed:
        return "Pressed";
    case Input::MouseAction::Released:
        return "Released";
    case Input::MouseAction::Dragged:
        return "Dragged";
    case Input::MouseAction::Clicked:
        return "Clicked";
    case Input::MouseAction::Wheel:
        return "Wheel";
    default:
        return "Unknown";
    }
}

std::string WidgetDemoScreen::describeWidget(const Widget* widget) const
{
    if (widget == nullptr)
    {
        return "<none>";
    }

    if (widget == m_listBox)
    {
        return "ListBox";
    }

    if (widget == m_applyButton)
    {
        return "Apply Selection Button";
    }

    if (widget == m_resetButton)
    {
        return "Reset List Button";
    }

    if (widget == m_disabledButton)
    {
        return "Disabled Button";
    }

    if (widget == m_textView)
    {
        return "TextView";
    }

    if (widget == m_titleLabel)
    {
        return "Title Label";
    }

    if (widget == m_stateLabel)
    {
        return "Selection Label";
    }

    if (widget == m_headerPanel)
    {
        return "Header Panel";
    }

    if (widget == m_controlPanel)
    {
        return "Controls Panel";
    }

    if (widget == m_textPanel)
    {
        return "Text Panel";
    }

    return "Widget";
}

void WidgetDemoScreen::drawTooSmall(Surface& surface) const
{
    ScreenBuffer& buffer = surface.buffer();
    buffer.clear(ScreenStyle);

    buffer.writeString(2, 2, "WidgetDemoScreen needs a larger terminal.", AccentStyle);
    buffer.writeString(2, 4, "Recommended minimum: 88 x 30", MutedStyle);
}
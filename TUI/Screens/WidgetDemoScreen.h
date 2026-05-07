#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "Input/MouseEvent.h"
#include "Screens/Screen.h"
#include "UI/Panels/Panel.h"
#include "UI/Panels/StatusBar.h"
#include "UI/Widgets/Button.h"
#include "UI/Widgets/ContainerWidget.h"
#include "UI/Widgets/Label.h"
#include "UI/Widgets/ListBox.h"
#include "UI/Widgets/TextView.h"

class Surface;

namespace Input
{
    class Event;
    struct Command;
}

class WidgetDemoScreen final : public Screen
{
public:
    WidgetDemoScreen();
    ~WidgetDemoScreen() override = default;

    void onEnter() override;
    bool handleEvent(const Input::Event& event) override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    void buildWidgets();
    void configureStyles();
    void updateLayout(Surface& surface);
    void updateStatusBar();

    bool handleCommand(const Input::Command& command);
    bool handleMouseEvent(const Input::MouseEvent& mouseEvent);
    void activateSelectedListItem();
    void handleButtonActivation(const ButtonActivationResult& result);
    void syncSelectionMessage();
    void updateSelectionLabel();
    void drawMouseDiagnosticsPanel(Surface& surface) const;

    std::string fitToWidth(const std::string& text, int width) const;
    void writePaddedLine(Surface& surface, int x, int y, int width, const std::string& text, const Style& style) const;

    std::string describeMouseEvent(const Input::MouseEvent& mouseEvent) const;
    std::string describeMouseButton(Input::MouseButton button) const;
    std::string describeMouseAction(Input::MouseAction action) const;
    std::string describeWidget(const Widget* widget) const;

    void drawTooSmall(Surface& surface) const;

private:
    ContainerWidget m_root;

    Panel* m_headerPanel = nullptr;
    Panel* m_controlPanel = nullptr;
    Panel* m_textPanel = nullptr;

    Label* m_titleLabel = nullptr;
    Label* m_stateLabel = nullptr;
    ListBox* m_listBox = nullptr;
    Button* m_applyButton = nullptr;
    Button* m_resetButton = nullptr;
    Button* m_disabledButton = nullptr;
    TextView* m_textView = nullptr;

    StatusBar m_statusBar;

    Rect m_diagnosticsBounds;

    std::string m_lastAction;
    std::string m_lastMouseSummary;
    std::string m_lastTargetSummary;
    std::string m_lastFocusSummary;
    std::string m_lastResultSummary;
    std::string m_lastMouseAction;
    std::optional<std::size_t> m_lastObservedSelection;
};
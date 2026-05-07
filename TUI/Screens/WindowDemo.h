#pragma once

#include <string>
#include <vector>

#include "Screens/Screen.h"
#include "UI/Base/WindowManager.h"
#include "UI/Panels/PopupWindow.h"
#include "UI/Panels/Window.h"

class WindowDemo final : public Screen
{
public:
    WindowDemo();

    void onEnter() override;
    bool handleEvent(const Input::Event& event) override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    class DemoWindow final : public Window
    {
    public:
        DemoWindow(const Rect& bounds, std::string title);

        void setLines(std::vector<std::string> lines);
        void draw(Surface& surface) override;

    private:
        std::vector<std::string> m_lines;
    };

    class DemoPopup final : public PopupWindow
    {
    public:
        DemoPopup(const Rect& bounds, std::string title);

        void setLines(std::vector<std::string> lines);
        void draw(Surface& surface) override;

    private:
        std::vector<std::string> m_lines;
    };

private:
    void configureWindows();
    void layoutWindows(const Surface& surface);

private:
    WindowManager m_windowManager;

    DemoWindow m_leftWindow;
    DemoWindow m_rightWindow;
    DemoWindow m_statusWindow;
    DemoPopup m_modalPopup;

    double m_elapsedSeconds = 0.0;
    int m_previousStage = -1;
};
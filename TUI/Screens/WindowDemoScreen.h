#pragma once

#include <memory>

#include "Screens/Screen.h"
#include "UI/Base/WindowManager.h"
#include "UI/Layout/DockTree.h"
#include "UI/Panels/EffectWindow.h"
#include "Rendering/Effects/FireEffect.h"
#include "Rendering/Effects/DigitalRainEffect.h"
#include "Rendering/Effects/Donut3D.h"
#include "Rendering/Effects/WaterWaveEffect.h"

class Surface;

class WindowDemo : public Screen
{
public:
    WindowDemo();
    ~WindowDemo() override;

    void onEnter() override;
    bool handleEvent(const Input::Event& event) override;
    void onExit() override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    void ensureLayout(const Rect& viewport, const Rect* windowRectArr);
    void ensureDockPreviewLayout(const Rect& viewport);

private:
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    bool m_layoutInitialized = false;

    // Effects
    DigitalRainEffect m_digiRain;
    Donut3D m_donut;
    FireEffect m_fire;
    WaterWaveEffect m_water;

    // Docking preview test layout
    UI::DockTree m_dockTree;

    // Window system
    WindowManager m_windowManager;
    std::unique_ptr<EffectWindow> m_digiRainWindow;
    std::unique_ptr<EffectWindow> m_donutWindow;
    std::unique_ptr<EffectWindow> m_fireWindow;
    std::unique_ptr<EffectWindow> m_waterWindow;
};
#pragma once

#include <string>

#include "Core/Rect.h"
#include "Screens/Screen.h"
#include "UI/Panels/Window.h"
#include "UI/Panels/EffectWindow.h"
#include "UI/Panels/ContentWindow.h"
#include "Rendering/Effects/Donut3D.h"
#include "Rendering/Objects/TextObject.h"
#include "UI/Base/WindowManager.h"
#include "UI/Layout/DockTree.h"

class Surface;
class Style;

class Donut3DScreen : public Screen
{
public:
    Donut3DScreen();
    ~Donut3DScreen();

    void onEnter() override;
    void onExit() override;
    bool handleEvent(const Input::Event& event) override;
    void update(const Animation::TickEvent& event) override;
    void draw(Surface& surface) override;

private:

    void ensureLayout(const Rect& viewport, const Rect* windowRectArr);
    void ensureDockContentModel(const Rect& viewport);
    void invalidateStaticUiCache();
    void ensureStaticUiCache(int screenWidth, int screenHeight);
    void rebuildStaticUiCache(int screenWidth, int screenHeight);
    bool isBelowMinimumScreenSize(int screenWidth, int screenHeight) const;

private:
    std::string m_titleText = "[ 3D ASCII Render Lab ]";
    std::string m_minimumSizeMessage = "3D Donut needs a larger console window.";
    bool m_layoutInitialized = false;
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    TextObject m_outerFrameObject;
    TextObject m_previewPaneObject;
    TextObject m_renderInfoPaneObject;
    TextObject m_footerPaneObject;
    TextObject m_titleObject;
    TextObject m_minimumSizeWarningObject;

    UI::DockTree m_dockTree;
    WindowManager m_windowManager;

    int m_cachedScreenWidth = -1;
    int m_cachedScreenHeight = -1;

    std::string m_cachedTitleText;
    std::string m_cachedFooterText;
    std::string m_cachedMinimumSizeMessage;

    int m_titleX = 4;
    int m_titleY = 0;

    int m_warningX = 1;
    int m_warningY = 1;

    Donut3D m_Donut3D;
    std::unique_ptr<EffectWindow> m_previewWindow;
    std::unique_ptr<UI::ContentWindow> m_renderInfoWindow;

    const char* m_previewWindowTitle = "( Preview )";
    const char* m_renderInfoWindowTitle = "( Render Info )";
};
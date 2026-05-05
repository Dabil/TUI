#pragma once

#include <string>

#include "Screens/Screen.h"
#include "Rendering/Effects/Donut3D.h"
#include "Rendering/Objects/TextObject.h"

class Surface;
class Style;

class Donut3DScreen : public Screen
{
public:
    Donut3DScreen();
    ~Donut3DScreen();

    void onEnter() override;
    void onExit() override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:

    void invalidateStaticUiCache();
    void ensureStaticUiCache(int screenWidth, int screenHeight);
    void rebuildStaticUiCache(int screenWidth, int screenHeight);
    bool isBelowMinimumScreenSize(int screenWidth, int screenHeight) const;

private:
    std::string m_titleText = "[ 3D ASCII Render Lab ]";
    std::string m_minimumSizeMessage = "3D Donut needs a larger console window.";

    TextObject m_outerFrameObject;
    TextObject m_previewPaneObject;
    TextObject m_renderInfoPaneObject;
    TextObject m_footerPaneObject;
    TextObject m_titleObject;
    TextObject m_minimumSizeWarningObject;

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
};
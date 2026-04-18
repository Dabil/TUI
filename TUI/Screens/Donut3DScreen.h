#pragma once

#include <string>
#include <vector>

#include "Rendering/Objects/TextObject.h"
#include "Screens/Screen.h"

class Surface;
class Style;

class Donut3DScreen : public Screen
{
public:
    Donut3DScreen();
    ~Donut3DScreen() override = default;

    void onEnter() override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    void ensureBuffers(int width, int height);
    void renderDonut(Surface& surface, int left, int top, int width, int height);

    void invalidateStaticUiCache();
    void ensureStaticUiCache(int screenWidth, int screenHeight);
    void rebuildStaticUiCache(int screenWidth, int screenHeight);

    bool isBelowMinimumScreenSize(int screenWidth, int screenHeight) const;

    int index(int x, int y, int width) const;
    Style buildShadedStyle(float normalizedDepth, float luminance) const;

private:
    double m_elapsedSeconds = 0.0;
    float m_rotationA = 0.0f;
    float m_rotationB = 0.0f;

    std::vector<float> m_depthBuffer;
    std::vector<char32_t> m_glyphBuffer;

    std::string m_titleText = "[ 3D ASCII Donut ]";
    std::string m_footerText = "[ Hue Cycle + Depth Shading + Floor Shadow ]";
    std::string m_minimumSizeMessage = "3D Donut needs a larger console window.";

    TextObject m_outerFrameObject;
    TextObject m_titleObject;
    TextObject m_footerObject;
    TextObject m_minimumSizeWarningObject;

    int m_cachedScreenWidth = -1;
    int m_cachedScreenHeight = -1;

    std::string m_cachedTitleText;
    std::string m_cachedFooterText;
    std::string m_cachedMinimumSizeMessage;

    int m_titleX = 4;
    int m_titleY = 0;

    int m_footerX = 4;
    int m_footerY = 0;

    int m_warningX = 1;
    int m_warningY = 1;
};
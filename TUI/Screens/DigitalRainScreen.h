#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Screens/Screen.h"
#include "Screens/Content/DigitalRainEffect.h"
#include "Rendering/Diagnostics/StartupDiagnosticsContext.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

class Surface;
class ScreenBuffer;

class rainXRay
{
public:
    rainXRay();
   ~rainXRay();

    void draw(ScreenBuffer& buffer);
    void move(int screenWidth, int screenHeight, double deltaTime);
    void reset(int screenWidth, int screenHeight);
    void invalidateXray();
    bool initialized();

private:
    float m_xRayX;
    float m_xRayY;
    float m_xRayDx;
    float m_xRayDy;

    int m_xRayWidth;
    int m_xRayHeight;

    int m_TopBounds;
    int m_BottomBounds;
    int m_LeftBounds;
    int m_RightBounds;

    bool m_xRay_initialized = false;

    TextObject m_RainXRayBox;
    TextObject m_RainXRayPanel;

    Style m_xRayBoxStyle;
    Style m_xRayPanelStyle;
};

class DigitalRainScreen : public Screen
{
public:
    ~DigitalRainScreen() override = default;
    explicit DigitalRainScreen(TerminalHostKind hostKind = TerminalHostKind::Unknown);

    void onEnter() override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    void ensureLayout(int screenWidth, int screenHeight);
    void ensureStaticUiCache();
    void ensureMinimumScreenUiCache(int screenWidth, int screenHeight);
    void invalidateStaticUiCache();
    void invalidateMinimumScreenUiCache();

    void updatePreview(double deltaTime);
    void drawPreviewLine(ScreenBuffer& buffer, int x, int y, int availableWidth, int startIndex) const;

    TextObject buildStaticUiTextObject() const;
    TextObject buildMinimumScreenTextObject(int screenWidth, int screenHeight) const;
    TextObject buildUnicodeContractPanelTextObject() const;

    std::vector<std::u32string> buildUnicodeContractPanelRows() const;

    std::u32string buildTitleText() const;
    std::u32string buildFooterDescriptionText() const;
    bool usesConsoleFooter() const;

private:
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    double m_elapsedSeconds = 0.0;
    double m_previewAdvanceTimer = 0.0;
    int m_previewOffset = 0;

    TerminalHostKind m_hostKind = TerminalHostKind::Unknown;

    Style m_backgroundStyle;
    Style m_titleStyle;
    Style m_labelStyle;
    Style m_previewStyle;
    Style m_borderStyle;
    Style m_windowStyle;
    Style m_xRayBoxStyle;
    Style m_xRayPanelStyle;

    bool m_staticUiDirty = true;
    bool m_minimumScreenUiDirty = true;
    int m_minimumScreenUiWidth = 0;
    int m_minimumScreenUiHeight = 0;
    bool m_cachedConsoleFooter = false;
    std::u32string m_cachedTitleText;
    std::u32string m_cachedFooterDescriptionText;
    TextObject m_staticUiObject;
    TextObject m_minimumScreenUiObject;

    rainXRay m_xRayMachine;

    // Remember: Member objects are constructed 
    // BEFORE the constructor body runs
    // so m_DigitalRain must be added to the
    // DigitralRainScreen initializer list
    DigitalRainEffect m_DigitalRain;
};

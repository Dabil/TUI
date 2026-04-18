#pragma once

#include <string>
#include <vector>

#include "Screens/Screen.h"
#include "Assets/AssetLibrary.h"
#include "Rendering/Composition/WritePolicy.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Objects/TextObject.h"

class Surface;
class Style;

class FireScreen : public Screen
{
public:
    explicit FireScreen(Assets::AssetLibrary& assetLibrary);
    ~FireScreen() override = default;

    void onEnter() override;
    void onExit() override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    void ensureSimulationSize(int width, int height);
    void seedFireSource();
    void updateFire(double deltaTime);
    void coolTopRows();

    void invalidateStaticUiCache();
    void ensureStaticUiObjects(int screenWidth, int screenHeight);

    TextObject buildBarObject(const std::string& labelText) const;
    TextObject buildFontLoadFailureObject(const std::string& text) const;
    TextObject buildMinimumSizeWarningObject(const std::string& text) const;

    int index(int x, int y) const;
    char32_t glyphForIntensity(int intensity) const;
    Style styleForIntensity(int intensity) const;

private:
    Assets::AssetLibrary& m_assetLibrary;

    const Style m_bannerStyle = style::Fg(Color::FromBasic(Color::Basic::Yellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style m_bannerStyleShadow = style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style m_outerFrameStyle = style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style m_borderTextStyle = style::Fg(Color::FromBasic(Color::Basic::Yellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Composition::WritePolicy m_solidUiWritePolicy = Composition::WritePresets::solidObject();

    std::string m_fireBannerFontKey = "banner.fire_font_k";
    TextObject m_tuiFireLogoObject;
    Assets::LoadBannerFontResult m_fontResult;

    TextObject m_outerFrameObject;
    TextObject m_innerFrameObject;
    TextObject m_titleBarObject;
    TextObject m_footerBarObject;
    TextObject m_fontLoadFailureObject;
    TextObject m_minimumSizeWarningObject;

    int m_cachedUiScreenWidth = -1;
    int m_cachedUiScreenHeight = -1;

    std::string m_cachedTitleBarText;
    std::string m_cachedFooterBarText;
    std::string m_cachedFontFailureText;
    std::string m_cachedMinimumSizeWarningText;

    int m_fireLeft = 0;
    int m_fireTop = 0;
    int m_fireWidth = 0;
    int m_fireHeight = 0;

    double m_elapsedSeconds = 0.0;
    double m_sourceFlickerTimer = 0.0;

    std::vector<int> m_fireBuffer;
    std::vector<int> m_nextFireBuffer;
};
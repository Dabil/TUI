#pragma once

#include "Screens/Screen.h"
#include "Rendering/Effects/FireEffect.h"
#include "Assets/AssetLibrary.h"

namespace Assets
{
    class AssetLibrary;
}

class Surface;

class FireScreen : public Screen
{
public:
    explicit FireScreen(Assets::AssetLibrary& assetLibrary);
    ~FireScreen() override = default;

    void onEnter() override;
    void onExit() override;
    void update(const Animation::TickEvent& event) override;
    void draw(Surface& surface) override;

private:
    void invalidateStaticUiCache();
    void ensureStaticUiObjects(int screenWidth, int screenHeight);


    TextObject buildBarObject(const std::string& labelText) const;
    TextObject buildFontLoadFailureObject(const std::string& text) const;
    TextObject buildMinimumSizeWarningObject(const std::string& text) const;

private:
    Assets::AssetLibrary& m_assetLibrary;

    Style m_bannerStyle;
    Style m_bannerStyleShadow;
    Style m_outerFrameStyle;
    Style m_borderTextStyle;

    Composition::WritePolicy m_solidUiWritePolicy;

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

    FireEffect m_fireEffect;
};
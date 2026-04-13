#pragma once

#include <vector>

#include "Screens/Screen.h"
#include "Assets/AssetLibrary.h"
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

    int index(int x, int y) const;
    char32_t glyphForIntensity(int intensity) const;
    Style styleForIntensity(int intensity) const;

private:
    Assets::AssetLibrary& m_assetLibrary;

    const Style m_bannerStyle = style::Fg(Color::FromBasic(Color::Basic::Yellow))
                               + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style m_bannerStyleShadow = style::Fg(Color::FromBasic(Color::Basic::Red))
                                    + style::Bg(Color::FromBasic(Color::Basic::Black));

    std::string m_tuiFireLogoKey = "banner.fireFontK";
    TextObject m_tuiFireLogoObject;
    Assets::LoadBannerFontResult m_fontResult;

    int m_fireLeft = 0;
    int m_fireTop = 0;
    int m_fireWidth = 0;
    int m_fireHeight = 0;

    double m_elapsedSeconds = 0.0;
    double m_sourceFlickerTimer = 0.0;

    std::vector<int> m_fireBuffer;
    std::vector<int> m_nextFireBuffer;
};
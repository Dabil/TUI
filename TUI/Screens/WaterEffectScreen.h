#pragma once

#include <string>
#include <vector>

#include "Screens/Screen.h"
#include "Rendering/Objects/pFontLoader.h"
#include "Rendering/Objects/LayeredTextObject.h"
#include "Rendering/Styles/Style.h"

class Surface;
class ScreenBuffer;

class WaterEffectScreen : public Screen
{
public:
    WaterEffectScreen();
    ~WaterEffectScreen() override = default;

    void onEnter() override;
    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    struct Droplet
    {
        float x = 0.0f;
        float y = 0.0f;
        float age = 0.0f;
        float lifetime = 0.0f;
        float speed = 0.0f;
        float wavelength = 0.0f;
        float strength = 0.0f;
    };

    enum class RainMode
    {
        Sprinkle,
        LightRain,
        Pouring
    };

private:
    void ensureSimulationSize(int width, int height);
    void rebuildTextMask();
    void spawnRandomDroplet();
    void updateDroplets(double deltaTime);
    void updateRainTiming();
    void chooseNextRainMode();

    void renderWaveField(Surface& surface);

    void ensureWaterTitleLoaded();
    void rebuildWaterTitle();
    void updateWaterTitleAnimation();
    void setWaterTitleLayerVisibility(bool stage1Visible, bool stage2Visible, bool stage3Visible, bool finalVisible);
    void hideWaterTitleLayers();
    void drawWaterTitle(ScreenBuffer& buffer) const;

    int index(int x, int y) const;

    int computeAmplitudeAtCell(int x, int y) const;
    char32_t selectWaveGlyph(int amplitude, char32_t sourceGlyph) const;
    Style selectWaveStyle(int amplitude, char32_t sourceGlyph) const;

private:
    int m_waveLeft = 0;
    int m_waveTop = 0;
    int m_waveWidth = 0;
    int m_waveHeight = 0;

    double m_elapsedSeconds = 0.0;
    double m_nextDropTime = 0.0;
    double m_nextRainModeChangeTime = 0.0;

    RainMode m_rainMode = RainMode::Sprinkle;
    Style m_modeColor[3];

    std::u32string m_primaryText = U"WATER EFFECT";
    std::u32string m_secondaryText = U"THE TEXT BECOMES THE WAVE";

    std::vector<char32_t> m_textMask;
    std::vector<Droplet> m_droplets;

    PseudoFont::FontDefinition m_waterTitleFont;
    Rendering::LayeredTextObject m_waterTitleObject;
    bool m_waterTitleLoadAttempted = false;
    bool m_waterTitleLoaded = false;
    std::string m_waterTitleLoadError;
};
#pragma once

#include <string>
#include <vector>

#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"

class Surface;

enum class RainMode
{
    Sprinkle,
    LightRain,
    Pouring
};

class WaterWaveEffect
{
public:
	WaterWaveEffect() = default;
	~WaterWaveEffect() = default;

	void onEnter();
	void onExit();
	void update(double deltaTime);
	void draw(Surface& surface, const Rect& viewport);
    RainMode getCurrentRainMode() const;

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

private:
    // Simulation Related
    void ensureLayout(const Rect& viewport);
    void ensureSimulationSize(int width, int height);
    void rebuildTextMask();
    void spawnRandomDroplet();
    void updateDroplets(double deltaTime);
    void updateRainTiming();
    void chooseNextRainMode();
    void renderWaveField(Surface& surface);
    int index(int x, int y) const;
    int computeAmplitudeAtCell(int x, int y) const;
    char32_t selectWaveGlyph(int amplitude, char32_t sourceGlyph) const;
    Style selectWaveStyle(int amplitude, char32_t sourceGlyph) const;

private:
    // Simulation related
    int m_waveLeft = 0;
    int m_waveTop = 0;
    int m_waveWidth = 0;
    int m_waveHeight = 0;

    double m_elapsedSeconds = 0.0;
    double m_nextDropTime = 0.0;
    double m_nextRainModeChangeTime = 0.0;

    RainMode m_rainMode = RainMode::Sprinkle;

    std::u32string m_primaryText = U"WATER EFFECT";
    std::u32string m_secondaryText = U"THE TEXT BECOMES THE WAVE";

    std::vector<char32_t> m_textMask;
    std::vector<Droplet> m_droplets;
};
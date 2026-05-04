#pragma once

#include <vector>

#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"

class Surface;

class FireEffect
{
public:
    explicit FireEffect();

    void onEnter();
    void onExit();
    void update(double deltaTime);
    void draw(Surface& surface, const Rect& viewport);

private:
    void setViewport(const Rect& viewport);
    void ensureSimulationSize(int width, int height);
    void seedFireSource();
    void updateFire(double deltaTime);
    void coolTopRows();

    int index(int x, int y) const;
    char32_t glyphForIntensity(int intensity) const;
    Style styleForIntensity(int intensity) const;

private:
    
    int m_fireLeft = 0;
    int m_fireTop = 0;
    int m_fireWidth = 0;
    int m_fireHeight = 0;
    
    double m_elapsedSeconds = 0.0;
    double m_sourceFlickerTimer = 0.0;

    std::vector<int> m_fireBuffer;
    std::vector<int> m_nextFireBuffer;
};
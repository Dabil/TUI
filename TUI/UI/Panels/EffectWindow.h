#pragma once

#include <string>

#include "UI/Panels/Window.h"
#include "Rendering/Effects/IEffects.h"

class EffectWindow : public Window
{
public:
    EffectWindow(Rect bounds, std::string title, IEffect& effect);

    void update(double deltaTime) override;
    void draw(Surface& surface) override;

private:
    IEffect& m_effect;
};
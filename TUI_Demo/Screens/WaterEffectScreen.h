#pragma once

#include <string>
#include <vector>

#include "Animation/AnimatedTextAssetSequence.h"
#include "Animation/AnimationBindingResolver.h"
#include "Animation/Animator.h"
#include "Animation/PseudoFontAnimationSequence.h"
#include "Assets/AssetLibrary.h"
#include "Rendering/Effects/WaterWaveEffect.h"
#include "Rendering/Objects/pFontLoader.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"
#include "Screens/Screen.h"

class Surface;
class ScreenBuffer;

class WaterEffectScreen : public Screen
{
public:
    explicit WaterEffectScreen(Assets::AssetLibrary& assetLibrary);
    ~WaterEffectScreen() override = default;

    void onEnter() override;
    void onExit() override;
    void update(const Animation::TickEvent& event) override;
    void draw(Surface& surface) override;

private:
    // Title related
    void ensureWaterTitleLoaded();
    void rebuildWaterTitleAnimation();
    std::vector<Animation::PseudoFontAnimationFrame> buildWaterTitleAnimationFrames() const;
    void drawWaterTitle(ScreenBuffer& buffer);

    // Static UI cache / layout related
    void ensureLayout(int screenWidth, int screenHeight);

    void invalidateStaticUiCache();
    void invalidateModeBarCache();
    void invalidateMinimumScreenUiCache();
    void invalidateWaterTitleFallbackCache();

    void ensureStaticUiCache();
    void ensureModeBarCache();
    void ensureMinimumScreenUiCache();
    void ensureWaterTitleFallbackCache();

    TextObject buildStaticUiTextObject() const;
    TextObject buildModeBarTextObject() const;
    TextObject buildMinimumScreenTextObject() const;
    TextObject buildWaterTitleFallbackTextObject() const;

    std::string currentModeTextUtf8() const;
    Style currentModeStyle() const;

private:
    // Title related
    Assets::AssetLibrary& m_assetLibrary;
    PseudoFont::FontDefinition m_waterTitleFont;

    Animation::AnimatedTextAssetSequence m_waterTitleSequence;
    Animation::Animator m_waterTitleAnimator;
    Animation::AnimationBindingResolver m_waterTitleBindingResolver;

    std::string m_waterTitlePseudoFontKey = "pfont.assemble_box";
    std::string m_waterTitleLoadError;
    bool m_waterTitleLoadAttempted = false;
    bool m_waterTitleLoaded = false;
    bool m_waterTitleAnimationReady = false;

    // Retained static UI cache
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    bool m_staticUiDirty = true;
    bool m_modeBarDirty = true;
    bool m_minimumScreenUiDirty = true;
    bool m_waterTitleFallbackDirty = true;

    RainMode m_cachedModeBarMode = RainMode::Sprinkle;
    Style m_modeColor[3];
    bool m_cachedWaterTitleLoaded = false;
    std::string m_cachedWaterTitleLoadError;

    TextObject m_staticUiObject;
    TextObject m_modeBarObject;
    TextObject m_minimumScreenUiObject;
    TextObject m_waterTitleFallbackObject;

    WaterWaveEffect m_waveEffect;
};
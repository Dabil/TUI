#include "Screens/WaterEffectScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Core/Rect.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/Surface.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/BannerThemes.h"
#include "Rendering/Objects/BannerFactory.h"
#include "Rendering/Objects/TextObjectComposer.h"
#include "Rendering/Objects/TextObjectFactory.h"

//TODO:: Seek and destroy unnecessary lambdas

namespace
{
    using Composition::Alignment;
    using Composition::PageComposer;

    constexpr double WaterTitleAssembleDurationSeconds = 1.5;
    constexpr double WaterTitleHoldDurationSeconds = 5.0;
    constexpr double WaterTitleReverseDurationSeconds = 1.5;
    constexpr double WaterTitleRepeatDelaySeconds = 10.0;
    constexpr double WaterTitleCycleDurationSeconds =
        WaterTitleAssembleDurationSeconds +
        WaterTitleHoldDurationSeconds +
        WaterTitleReverseDurationSeconds +
        WaterTitleRepeatDelaySeconds;
}

namespace WaterColors
{
    inline const Style BackGround =
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style TitleColor =
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style TitleColorUnderlined =
          style::Bold
        + style::Underline
        + style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style BorderColor =
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style SprinkleColor =
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::Green))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style LightRainColor =
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::Blue))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style PouringColor =
          style::SlowBlink
        + style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style WaterTitleLoadErrorColor =
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));
}

WaterEffectScreen::WaterEffectScreen(Assets::AssetLibrary& assetLibrary)
    : m_assetLibrary(assetLibrary)
{
    m_modeColor[0] = WaterColors::SprinkleColor;
    m_modeColor[1] = WaterColors::LightRainColor;
    m_modeColor[2] = WaterColors::PouringColor;
}

void WaterEffectScreen::onEnter()
{
    m_elapsedSeconds = 0.0;
    invalidateStaticUiCache();
    invalidateModeBarCache();
    invalidateMinimumScreenUiCache();
    invalidateWaterTitleFallbackCache();

    ensureWaterTitleLoaded();
    rebuildWaterTitle();
    updateWaterTitleAnimation();

    m_waveEffect.onEnter();
}

void WaterEffectScreen::onExit()
{
    m_waterTitleObject.clear();
    invalidateWaterTitleFallbackCache();
    m_waveEffect.onExit();
}

void WaterEffectScreen::update(double deltaTime)
{
    m_elapsedSeconds += deltaTime;
    updateWaterTitleAnimation();

    if (m_screenWidth <= 2 || m_screenHeight <= 2)
    {
        return;
    }

    m_waveEffect.update(deltaTime);
}

void WaterEffectScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    const int screenWidth = buffer.getWidth();
    const int screenHeight = buffer.getHeight();

    if (screenWidth < 30 || screenHeight < 12)
    {
        ensureMinimumScreenUiCache();
        m_minimumScreenUiObject.draw(buffer, 1, 1);
        return;
    }

    ensureLayout(screenWidth, screenHeight);
    ensureStaticUiCache();
    ensureModeBarCache();

    m_staticUiObject.draw(buffer, 0, 0, Composition::WritePresets::solidObject());
    m_modeBarObject.draw(buffer, 4, m_screenHeight - 1, Composition::WritePresets::solidObject());

    PageComposer page(buffer);
    page.clearRegions();

    page.createFullScreenRegion("Screen");
    page.createInsetRegion("WaveWaterEffect", "Screen", 3, 2, 3, 2);

    const Rect& viewport = page.resolveRegion("WaveWaterEffect");

    m_waveEffect.draw(surface, viewport);
    drawWaterTitle(buffer);
}

void WaterEffectScreen::ensureLayout(int screenWidth, int screenHeight)
{
    const bool screenSizeChanged =
        (screenWidth != m_screenWidth) ||
        (screenHeight != m_screenHeight);

    if (!screenSizeChanged)
    {
        return;
    }

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    invalidateStaticUiCache();
    invalidateModeBarCache();
}

void WaterEffectScreen::invalidateStaticUiCache()
{
    m_staticUiDirty = true;
}

void WaterEffectScreen::invalidateModeBarCache()
{
    m_modeBarDirty = true;
}

void WaterEffectScreen::invalidateMinimumScreenUiCache()
{
    m_minimumScreenUiDirty = true;
}

void WaterEffectScreen::invalidateWaterTitleFallbackCache()
{
    m_waterTitleFallbackDirty = true;
}

void WaterEffectScreen::ensureStaticUiCache()
{
    if (!m_staticUiDirty)
    {
        return;
    }

    m_staticUiObject = buildStaticUiTextObject();
    m_staticUiDirty = false;
}

void WaterEffectScreen::ensureModeBarCache()
{
    RainMode currentRainMode = m_waveEffect.getCurrentRainMode();
    if (!m_modeBarDirty && m_cachedModeBarMode == currentRainMode)
    {
        return;
    }

    m_modeBarObject = buildModeBarTextObject();
    m_cachedModeBarMode = currentRainMode;
    m_modeBarDirty = false;
}

void WaterEffectScreen::ensureMinimumScreenUiCache()
{
    if (!m_minimumScreenUiDirty && !m_minimumScreenUiObject.isEmpty())
    {
        return;
    }

    m_minimumScreenUiObject = buildMinimumScreenTextObject();
    m_minimumScreenUiDirty = false;
}

void WaterEffectScreen::ensureWaterTitleFallbackCache()
{
    const bool stateChanged =
        (m_cachedWaterTitleLoaded != m_waterTitleLoaded) ||
        (m_cachedWaterTitleLoadError != m_waterTitleLoadError);

    if (!m_waterTitleFallbackDirty && !stateChanged)
    {
        return;
    }

    m_cachedWaterTitleLoaded = m_waterTitleLoaded;
    m_cachedWaterTitleLoadError = m_waterTitleLoadError;
    m_waterTitleFallbackObject = buildWaterTitleFallbackTextObject();
    m_waterTitleFallbackDirty = false;
}

TextObject WaterEffectScreen::buildStaticUiTextObject() const
{
    if (m_screenWidth <= 0 || m_screenHeight <= 0)
    {
        return TextObject();
    }

    TextObjectComposer composer;

    composer.addFilledRect(
        0,
        0,
        m_screenWidth,
        m_screenHeight,
        U' ',
        WaterColors::BackGround,
        0,
        "Background");
    
    composer.addFrame(
        0,
        0,
        m_screenWidth,
        m_screenHeight,
        WaterColors::BorderColor,
        ObjectFactory::roundedBorder(),
        10,
        "outerFrame");

    composer.addFrame(
        2,
        1,
        m_screenWidth - 4,
        m_screenHeight - 2,
        WaterColors::BorderColor,
        ObjectFactory::roundedBorder(),
        10,
        "innerFrame");

    composer.addTextUtf8(
        "[                         ]",
        4,
        0,
        WaterColors::TitleColor,
        20,
        "titleBarBack");

    composer.addTextUtf8(
        " Rain Drops Water Effect ",
        5,
        0,
        WaterColors::TitleColorUnderlined,
        21,
        "titleBarText");
       
    TextObjectComposer::BuildTextObjectOptions options;
    options.writePolicy = Composition::WritePresets::solidObject();
    return composer.buildTextObject(options);
}

TextObject WaterEffectScreen::buildModeBarTextObject() const
{
    if (m_screenWidth <= 0 || m_screenHeight <= 0)
    {
        return TextObject();
    }

    const std::string modeText = currentModeTextUtf8();

    TextObjectComposer composer;
    composer.addTextUtf8(
        "[ Mode:",
        4,
        0,
        WaterColors::TitleColor,
        10,
        "modeBarPrefix");

    composer.addTextUtf8(
        modeText,
        11,
        0,
        currentModeStyle(),
        11,
        "modeBarModeText");

    composer.addGlyph(
        U']',
        11 + static_cast<int>(modeText.size()),
        0,
        WaterColors::TitleColor,
        12,
        "modeBarSuffix");

    TextObjectComposer::BuildTextObjectOptions options;
    options.writePolicy = Composition::WritePresets::solidObject();
    return composer.buildTextObject(options);
}

TextObject WaterEffectScreen::buildMinimumScreenTextObject() const
{
    return TextObject::fromUtf8(
        "WaterEffect needs a larger console window.",
        Themes::Warning);
}

TextObject WaterEffectScreen::buildWaterTitleFallbackTextObject() const
{
    if (m_waterTitleLoaded || m_waterTitleLoadError.empty())
    {
        return TextObject();
    }

    return TextObject::fromUtf8(
        "[Missing Assemble Box.pfont]",
        WaterColors::WaterTitleLoadErrorColor);
}

std::string WaterEffectScreen::currentModeTextUtf8() const
{
    switch (m_waveEffect.getCurrentRainMode())
    {
    case RainMode::Sprinkle:
        return " Sprinkle ";

    case RainMode::LightRain:
        return " Light Rain ";

    case RainMode::Pouring:
        return " Pouring ";
    }

    return " Sprinkle ";
}

Style WaterEffectScreen::currentModeStyle() const
{
    switch (m_waveEffect.getCurrentRainMode())
    {
    case RainMode::Sprinkle:
        return m_modeColor[static_cast<int>(RainMode::Sprinkle)];

    case RainMode::LightRain:
        return m_modeColor[static_cast<int>(RainMode::LightRain)];

    case RainMode::Pouring:
        return m_modeColor[static_cast<int>(RainMode::Pouring)];
    }

    return m_modeColor[static_cast<int>(RainMode::Sprinkle)];
}

void WaterEffectScreen::ensureWaterTitleLoaded()
{
    if (m_waterTitleLoadAttempted)
    {
        return;
    }

    m_waterTitleLoadAttempted = true;
    m_waterTitleLoaded = false;
    m_waterTitleLoadError.clear();

    const Assets::LoadPseudoFontAssetResult loadResult =
        m_assetLibrary.loadPseudoFontAsset(m_waterTitlePseudoFontKey);

    if (!loadResult.success || !loadResult.hasFont())
    {
        m_waterTitleLoadError = loadResult.errorMessage.empty()
            ? std::string("Failed to load water title pFont asset.")
            : loadResult.errorMessage;
        invalidateWaterTitleFallbackCache();
        return;
    }

    m_waterTitleFont = *loadResult.asset.font;
    m_waterTitleLoaded = true;
    invalidateWaterTitleFallbackCache();
}

void WaterEffectScreen::rebuildWaterTitle()
{
    if (!m_waterTitleLoaded)
    {
        m_waterTitleObject.clear();
        return;
    }

    PseudoFont::LayeredRenderOptions options;
    options.initialVisibilityMode =
        PseudoFont::LayeredRenderOptions::InitialVisibilityMode::AllHidden;

    m_waterTitleObject =
        BannerFactory::makeLayeredBannerObject(
            m_waterTitleFont,
            "WATER TUI",
            options);

    hideWaterTitleLayers();
}

void WaterEffectScreen::updateWaterTitleAnimation()
{
    if (!m_waterTitleLoaded || m_waterTitleObject.isEmpty())
    {
        return;
    }

    hideWaterTitleLayers();

    const double cycleTime = std::fmod(m_elapsedSeconds, WaterTitleCycleDurationSeconds);

    if (cycleTime < WaterTitleAssembleDurationSeconds)
    {
        const double stepDuration = WaterTitleAssembleDurationSeconds / 4.0;

        if (cycleTime < stepDuration)
        {
            setWaterTitleLayerVisibility(true, false, false, false);
        }
        else if (cycleTime < (stepDuration * 2.0))
        {
            setWaterTitleLayerVisibility(false, true, false, false);
        }
        else if (cycleTime < (stepDuration * 3.0))
        {
            setWaterTitleLayerVisibility(false, false, true, false);
        }
        else
        {
            setWaterTitleLayerVisibility(false, false, false, true);
        }

        return;
    }

    if (cycleTime < (WaterTitleAssembleDurationSeconds + WaterTitleHoldDurationSeconds))
    {
        setWaterTitleLayerVisibility(false, false, false, true);
        return;
    }

    if (cycleTime < (WaterTitleAssembleDurationSeconds + WaterTitleHoldDurationSeconds + WaterTitleReverseDurationSeconds))
    {
        const double reverseTime =
            cycleTime - (WaterTitleAssembleDurationSeconds + WaterTitleHoldDurationSeconds);
        const double stepDuration = WaterTitleReverseDurationSeconds / 4.0;

        if (reverseTime < stepDuration)
        {
            setWaterTitleLayerVisibility(false, false, false, true);
        }
        else if (reverseTime < (stepDuration * 2.0))
        {
            setWaterTitleLayerVisibility(false, false, true, false);
        }
        else if (reverseTime < (stepDuration * 3.0))
        {
            setWaterTitleLayerVisibility(false, true, false, false);
        }
        else
        {
            setWaterTitleLayerVisibility(true, false, false, false);
        }

        return;
    }

    hideWaterTitleLayers();
}

void WaterEffectScreen::setWaterTitleLayerVisibility(
    bool stage1Visible,
    bool stage2Visible,
    bool stage3Visible,
    bool finalVisible)
{
    m_waterTitleObject.setLayerVisibility("stage_1", stage1Visible);
    m_waterTitleObject.setLayerVisibility("stage_2", stage2Visible);
    m_waterTitleObject.setLayerVisibility("stage_3", stage3Visible);
    m_waterTitleObject.setLayerVisibility("final", finalVisible);
}

void WaterEffectScreen::hideWaterTitleLayers()
{
    setWaterTitleLayerVisibility(false, false, false, false);
}

void WaterEffectScreen::drawWaterTitle(ScreenBuffer& buffer) const
{
    if (!m_waterTitleLoaded)
    {
        const_cast<WaterEffectScreen*>(this)->ensureWaterTitleFallbackCache();

        if (!m_waterTitleFallbackObject.isEmpty())
        {
            const int x = std::max(1, (buffer.getWidth() - m_waterTitleFallbackObject.getWidth()) / 2);
            const int y = std::max(2, (buffer.getHeight() / 3) - 1);
            m_waterTitleFallbackObject.draw(buffer, x, y);
        }

        return;
    }

    const TextObject waterTitle = m_waterTitleObject.flattenVisibleOnly();
    if (waterTitle.isEmpty())
    {
        return;
    }

    const int x = std::max(0, (buffer.getWidth() - waterTitle.getWidth()) / 2);
    const int y = std::max(2, (buffer.getHeight() / 3) - (waterTitle.getHeight() / 2));

    waterTitle.draw(buffer, x, y, Composition::WritePresets::visibleObject());
}
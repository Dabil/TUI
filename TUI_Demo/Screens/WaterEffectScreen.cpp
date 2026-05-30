#include "Screens/WaterEffectScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

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

    constexpr const char* WaterTitleControllerName = "water.title.controller";
    constexpr const char* WaterTitleTargetName = "water.title";
    constexpr const char* WaterTitleRegionName = "WaterTitleRegion";

    constexpr double WaterTitleAssembleDurationSeconds = 1.5;
    constexpr double WaterTitleHoldDurationSeconds = 5.0;
    constexpr double WaterTitleReverseDurationSeconds = 1.5;
    constexpr double WaterTitleRepeatDelaySeconds = 10.0;

    std::string buildPseudoFontAnimationErrorMessage(
        const Animation::PseudoFontAnimationSequenceBuildResult& result)
    {
        std::ostringstream stream;
        stream << "Failed to build WATER pFont animation sequence.";

        for (const Animation::PseudoFontAnimationFrameDiagnostic& diagnostic : result.diagnostics)
        {
            if (diagnostic.success)
            {
                continue;
            }

            stream << " " << diagnostic.message;
        }

        return stream.str();
    }
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
    invalidateStaticUiCache();
    invalidateModeBarCache();
    invalidateMinimumScreenUiCache();
    invalidateWaterTitleFallbackCache();

    ensureWaterTitleLoaded();
    rebuildWaterTitleAnimation();

    m_waveEffect.onEnter();
}

void WaterEffectScreen::onExit()
{
    m_waterTitleBindingResolver.clear();
    m_waterTitleSequence.clear();
    m_waterTitleAnimator.stop();
    m_waterTitleAnimationReady = false;

    invalidateWaterTitleFallbackCache();
    m_waveEffect.onExit();
}

void WaterEffectScreen::update(const Animation::TickEvent& event)
{
    if (m_waterTitleAnimationReady)
    {
        m_waterTitleBindingResolver.updateBoundControllers(event);
    }

    if (m_screenWidth <= 2 || m_screenHeight <= 2)
    {
        return;
    }

    m_waveEffect.update(event);
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

void WaterEffectScreen::rebuildWaterTitleAnimation()
{
    m_waterTitleBindingResolver.clear();
    m_waterTitleSequence.clear();
    m_waterTitleAnimator.stop();
    m_waterTitleAnimationReady = false;

    if (!m_waterTitleLoaded)
    {
        return;
    }

    PseudoFont::LayeredRenderOptions options;
    options.initialVisibilityMode =
        PseudoFont::LayeredRenderOptions::InitialVisibilityMode::AllHidden;

    const Rendering::LayeredTextObject waterTitleObject =
        BannerFactory::makeLayeredBannerObject(
            m_waterTitleFont,
            "WATER TUI",
            options);

    const Animation::PseudoFontAnimationSequenceBuildResult buildResult =
        Animation::buildPseudoFontAnimationSequenceWithDiagnostics(
            waterTitleObject,
            buildWaterTitleAnimationFrames(),
            "WaterEffectScreen.WATER.title");

    if (!buildResult.success || buildResult.builtFrameCount == 0)
    {
        m_waterTitleLoaded = false;
        m_waterTitleLoadError = buildPseudoFontAnimationErrorMessage(buildResult);
        invalidateWaterTitleFallbackCache();
        return;
    }

    m_waterTitleSequence = buildResult.sequence;
    m_waterTitleSequence.setLooping(true);

    const double totalDurationSeconds = m_waterTitleSequence.totalDurationSeconds();
    if (totalDurationSeconds <= 0.0 || m_waterTitleSequence.frameCount() == 0)
    {
        m_waterTitleLoaded = false;
        m_waterTitleLoadError = "WATER pFont animation sequence has no playable duration.";
        invalidateWaterTitleFallbackCache();
        return;
    }

    const double framesPerSecond =
        static_cast<double>(m_waterTitleSequence.frameCount()) / totalDurationSeconds;

    m_waterTitleAnimator.configure(
        m_waterTitleSequence.frameCount(),
        framesPerSecond);

    m_waterTitleAnimator.setPlaybackMode(Animation::PlaybackMode::Loop);
    m_waterTitleAnimator.play();

    m_waterTitleBindingResolver.bindController(
        WaterTitleControllerName,
        m_waterTitleAnimator);

    m_waterTitleAnimationReady = true;
    invalidateWaterTitleFallbackCache();
}

std::vector<Animation::PseudoFontAnimationFrame>
WaterEffectScreen::buildWaterTitleAnimationFrames() const
{
    const double assembleStepSeconds = WaterTitleAssembleDurationSeconds / 4.0;
    const double reverseStepSeconds = WaterTitleReverseDurationSeconds / 4.0;

    std::vector<Animation::PseudoFontAnimationFrame> frames;

    frames.push_back({ "stage_1", { "stage_1" }, assembleStepSeconds });
    frames.push_back({ "stage_2", { "stage_2" }, assembleStepSeconds });
    frames.push_back({ "stage_3", { "stage_3" }, assembleStepSeconds });
    frames.push_back({ "final", { "final" }, assembleStepSeconds });

    frames.push_back({ "final_hold", { "final" }, WaterTitleHoldDurationSeconds });

    frames.push_back({ "reverse_final", { "final" }, reverseStepSeconds });
    frames.push_back({ "reverse_stage_3", { "stage_3" }, reverseStepSeconds });
    frames.push_back({ "reverse_stage_2", { "stage_2" }, reverseStepSeconds });
    frames.push_back({ "reverse_stage_1", { "stage_1" }, reverseStepSeconds });

    frames.push_back({ "hidden_pause", {}, WaterTitleRepeatDelaySeconds });

    return frames;
}

void WaterEffectScreen::drawWaterTitle(ScreenBuffer& buffer)
{
    if (!m_waterTitleLoaded || !m_waterTitleAnimationReady)
    {
        ensureWaterTitleFallbackCache();

        if (!m_waterTitleFallbackObject.isEmpty())
        {
            const int x = std::max(1, (buffer.getWidth() - m_waterTitleFallbackObject.getWidth()) / 2);
            const int y = std::max(2, (buffer.getHeight() / 3) - 1);
            m_waterTitleFallbackObject.draw(buffer, x, y);
        }

        return;
    }

    PageComposer page(buffer);
    page.clearRegions();

    const int titleRegionHeight = std::max(1, std::min(16, buffer.getHeight() - 2));
    const int titleCenterY = std::max(2, buffer.getHeight() / 3);

    int titleRegionTop = titleCenterY - (titleRegionHeight / 2);
    titleRegionTop = std::max(0, titleRegionTop);

    if (titleRegionTop + titleRegionHeight > buffer.getHeight())
    {
        titleRegionTop = std::max(0, buffer.getHeight() - titleRegionHeight);
    }

    page.createRegion(
        WaterTitleRegionName,
        Rect{
            Point{ 0, titleRegionTop },
            Size{ buffer.getWidth(), titleRegionHeight }
        });

    const Animation::AnimationBindingTarget target =
        Animation::AnimationBindingTarget::animatedTextSequence(
            WaterTitleTargetName,
            WaterTitleControllerName,
            m_waterTitleSequence,
            PageComposer::PlacementSpec::inNamedRegion(
                WaterTitleRegionName,
                Composition::Align::center(),
                true),
            Composition::WritePresets::visibleObject());

    m_waterTitleBindingResolver.setTargets({ target });
    m_waterTitleBindingResolver.resolveAll(page);
}
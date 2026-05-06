#include "Screens/FireScreen.h"

#include "Rendering/Surface.h"
#include "Core/Rect.h"
#include "Rendering/Objects/BannerFactory.h"
#include "Rendering/Objects/TextObjectComposer.h"
#include "Rendering/Objects/TextObjectFactory.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"


namespace
{
    constexpr int MinimumScreenWidth = 30;
    constexpr int MinimumScreenHeight = 12;

    const std::string kTitleBarText = " Fire Simulation ";
    const std::string kFooterBarText = " Buffered Flames ";
    const std::string kFontFailureText = "Failed to load banner font.";
    const std::string kMinimumSizeWarningText = "FireScreen needs a larger console window.";
}

FireScreen::FireScreen(Assets::AssetLibrary& assetLibrary)
    : m_assetLibrary(assetLibrary)
    , m_bannerStyle(
        style::Fg(Color::FromBasic(Color::Basic::Yellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black)))
    , m_bannerStyleShadow(
        style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Black)))
    , m_outerFrameStyle(
        style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Black)))
    , m_borderTextStyle(
        style::Fg(Color::FromBasic(Color::Basic::Yellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black)))
    , m_solidUiWritePolicy(Composition::WritePresets::solidObject())
{
}

void FireScreen::onEnter()
{
    m_fireEffect.onEnter();

    m_tuiFireLogoObject.clear();

    invalidateStaticUiCache();

    m_fontResult = m_assetLibrary.loadBannerFontAsset(m_fireBannerFontKey);

    if (!m_fontResult.success || !m_fontResult.hasFont())
    {
        return;
    }

    AsciiBanner::RenderOptions options = BannerFactory::kernOptions();
    options.transparentSpaces = true;

    m_tuiFireLogoObject = BannerFactory::makeShadowBanner(
        *m_fontResult.asset.font,
        "TUI",
        m_bannerStyle,
        m_bannerStyleShadow,
        options,
        0,
        -1);
}

void FireScreen::onExit()
{
    m_fireEffect.onExit();

    m_tuiFireLogoObject.clear();

    m_outerFrameObject.clear();
    m_innerFrameObject.clear();
    m_titleBarObject.clear();
    m_footerBarObject.clear();
    m_fontLoadFailureObject.clear();
    m_minimumSizeWarningObject.clear();

    invalidateStaticUiCache();
}

void FireScreen::update(double deltaTime)
{
    m_fireEffect.update(deltaTime);
}

void FireScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    const int screenWidth = buffer.getWidth();
    const int screenHeight = buffer.getHeight();

    ensureStaticUiObjects(screenWidth, screenHeight);

    if (screenWidth < MinimumScreenWidth || screenHeight < MinimumScreenHeight)
    {
        m_minimumSizeWarningObject.draw(buffer, 1, 1);
        return;
    }

    m_outerFrameObject.draw(buffer, 0, 0);
    m_innerFrameObject.draw(buffer, 2, 1);
    m_titleBarObject.draw(buffer, 4, 0, m_solidUiWritePolicy);
    m_footerBarObject.draw(buffer, 4, screenHeight - 1, m_solidUiWritePolicy);

    // Controls dimensions of fire simulation
    Rect contentArea = Rect{ Point{3, 2}, Size{screenWidth - 6, screenHeight - 4} };
    m_fireEffect.draw(surface, contentArea);

    if (!m_fontResult.success || !m_fontResult.hasFont())
    {
        const int errorX = (screenWidth - m_fontLoadFailureObject.getWidth()) / 2;
        const int errorY = screenHeight / 4;

        m_fontLoadFailureObject.draw(buffer, errorX, errorY);
        return;
    }

    const int bannerWidth = m_tuiFireLogoObject.getWidth();
    const int bannerHeight = m_tuiFireLogoObject.getHeight();

    const int bannerX = (screenWidth - bannerWidth) / 2;
    const int bannerY = (screenHeight - bannerHeight) / 4;

    m_tuiFireLogoObject.draw(buffer, bannerX - 1, bannerY - 1, Composition::WritePolicy::VisibleObject());
}

void FireScreen::invalidateStaticUiCache()
{
    m_cachedUiScreenWidth = -1;
    m_cachedUiScreenHeight = -1;

    m_cachedTitleBarText.clear();
    m_cachedFooterBarText.clear();
    m_cachedFontFailureText.clear();
    m_cachedMinimumSizeWarningText.clear();
}

void FireScreen::ensureStaticUiObjects(int screenWidth, int screenHeight)
{
    if (screenWidth != m_cachedUiScreenWidth || screenHeight != m_cachedUiScreenHeight)
    {
        m_cachedUiScreenWidth = screenWidth;
        m_cachedUiScreenHeight = screenHeight;

        if (screenWidth > 0 && screenHeight > 0)
        {
            m_outerFrameObject = ObjectFactory::makeFrame(
                screenWidth,
                screenHeight,
                m_outerFrameStyle,
                ObjectFactory::doubleLineBorder());
        }
        else
        {
            m_outerFrameObject.clear();
        }

        if (screenWidth >= 4 && screenHeight >= 2)
        {
            m_innerFrameObject = ObjectFactory::makeFrame(
                screenWidth - 4,
                screenHeight - 2,
                m_borderTextStyle,
                ObjectFactory::singleLineBorder());
        }
        else
        {
            m_innerFrameObject.clear();
        }
    }

    if (m_cachedTitleBarText != kTitleBarText || m_titleBarObject.isEmpty())
    {
        m_cachedTitleBarText = kTitleBarText;
        m_titleBarObject = buildBarObject(m_cachedTitleBarText);
    }

    if (m_cachedFooterBarText != kFooterBarText || m_footerBarObject.isEmpty())
    {
        m_cachedFooterBarText = kFooterBarText;
        m_footerBarObject = buildBarObject(m_cachedFooterBarText);
    }

    if (m_cachedFontFailureText != kFontFailureText || m_fontLoadFailureObject.isEmpty())
    {
        m_cachedFontFailureText = kFontFailureText;
        m_fontLoadFailureObject = buildFontLoadFailureObject(m_cachedFontFailureText);
    }

    if (m_cachedMinimumSizeWarningText != kMinimumSizeWarningText || m_minimumSizeWarningObject.isEmpty())
    {
        m_cachedMinimumSizeWarningText = kMinimumSizeWarningText;
        m_minimumSizeWarningObject = buildMinimumSizeWarningObject(m_cachedMinimumSizeWarningText);
    }
}

TextObject FireScreen::buildBarObject(const std::string& labelText) const
{
    TextObjectComposer composer;
    composer.addTextUtf8("[                 ]", 0, 0, m_outerFrameStyle);
    composer.addTextUtf8(labelText, 1, 0, m_borderTextStyle);

    TextObjectComposer::BuildTextObjectOptions options;
    options.visibleOnly = true;
    options.writePolicy = Composition::WritePresets::solidObject();

    return composer.buildTextObject(options);
}

TextObject FireScreen::buildFontLoadFailureObject(const std::string& text) const
{
    return TextObject::fromUtf8(text, Themes::Warning);
}

TextObject FireScreen::buildMinimumSizeWarningObject(const std::string& text) const
{
    return TextObject::fromUtf8(text, Themes::Warning);
}
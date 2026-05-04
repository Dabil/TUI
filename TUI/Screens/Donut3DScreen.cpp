#include "Screens/Donut3DScreen.h"

#include <cmath>

#include "Rendering/Surface.h"
#include "Rendering/ScreenBuffer.h"

#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Styles/StyleBuilder.h"

#include "Rendering/Objects/TextObject.h"
#include "Rendering/Objects/TextObjectFactory.h"
#include "Rendering/Composition/Alignment.h"
#include "Rendering/Composition/PageComposer.h"

namespace
{
    using Composition::Alignment;
    using Composition::PageComposer;
    using Composition::WritePresets::solidObject;
    using Composition::WritePresets::visibleObject;
    using Composition::WritePresets::authoredObject;

    constexpr int MinimumScreenWidth = 30;
    constexpr int MinimumScreenHeight = 12;
}

namespace DonutColors
{
    inline const Style Background =
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style BorderColor =
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));
}

Donut3DScreen::Donut3DScreen() = default;

Donut3DScreen::~Donut3DScreen()
{
    m_outerFrameObject.clear();
    m_previewPaneObject.clear();
    m_renderInfoPaneObject.clear();
    m_footerPaneObject.clear();
    m_titleObject.clear();
    m_minimumSizeWarningObject.clear();
}

void Donut3DScreen::onEnter()
{
    invalidateStaticUiCache();
    m_Donut3D.onEnter();
}

void Donut3DScreen::onExit()
{
    m_Donut3D.onExit();
}

void Donut3DScreen::update(double deltaTime)
{
    m_Donut3D.update(deltaTime);
}

void Donut3DScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    const int screenWidth = buffer.getWidth();
    const int screenHeight = buffer.getHeight();

    ensureStaticUiCache(screenWidth, screenHeight);

    if (isBelowMinimumScreenSize(screenWidth, screenHeight))
    {
        m_minimumSizeWarningObject.draw(buffer, m_warningX, m_warningY);
        return;
    }

    PageComposer page(buffer);
    page.clearRegions();

    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Safe", "Screen", 2, 1, 2, 1);

    const Rect safe = page.resolveRegion("Safe");
    const auto [footer, body] = page.splitBottom(safe, 5);
    const auto [rightPane, leftPane] = page.splitRight(body, body.size.width / 3);

    page.createRegion("LeftPane", leftPane);
    page.createRegion("RightPane", rightPane);
    page.createRegion("Footer", footer);

    page.createInsetRegion("3dViewPort", "LeftPane", 2, 1, 2, 1);
    const Rect viewPort = page.resolveRegion("3dViewPort");

    page.writeObjectInRegion(m_outerFrameObject, "Screen", Composition::Align::center());

    page.drawPanel("LeftPane", DonutColors::Background, DonutColors::BorderColor, ObjectFactory::roundedBorder());
    page.drawPanel("RightPane", DonutColors::Background, DonutColors::BorderColor, ObjectFactory::roundedBorder());
    page.drawPanel("Footer", DonutColors::Background, DonutColors::BorderColor, ObjectFactory::roundedBorder());

    m_titleObject.draw(buffer, m_titleX, m_titleY);
        
    m_Donut3D.draw(surface, viewPort);
}

void Donut3DScreen::invalidateStaticUiCache()
{
    m_cachedScreenWidth = -1;
    m_cachedScreenHeight = -1;

    m_cachedTitleText.clear();
    m_cachedFooterText.clear();
    m_cachedMinimumSizeMessage.clear();

    m_outerFrameObject.clear();
    m_previewPaneObject.clear();
    m_renderInfoPaneObject.clear();
    m_footerPaneObject.clear();
    m_titleObject.clear();
    m_minimumSizeWarningObject.clear();

    m_titleX = 4;
    m_titleY = 0;
    m_warningX = 1;
    m_warningY = 1;
}

void Donut3DScreen::ensureStaticUiCache(int screenWidth, int screenHeight)
{
    const bool sizeChanged =
        (screenWidth != m_cachedScreenWidth) ||
        (screenHeight != m_cachedScreenHeight);

    const bool contentChanged =
        (m_titleText != m_cachedTitleText) ||
        // (m_footerText != m_cachedFooterText) ||
        (m_minimumSizeMessage != m_cachedMinimumSizeMessage);

    if (!sizeChanged && !contentChanged)
    {
        return;
    }

    rebuildStaticUiCache(screenWidth, screenHeight);
}

void Donut3DScreen::rebuildStaticUiCache(int screenWidth, int screenHeight)
{
    m_cachedScreenWidth = screenWidth;
    m_cachedScreenHeight = screenHeight;

    m_cachedTitleText = m_titleText;
    m_cachedMinimumSizeMessage = m_minimumSizeMessage;

    m_outerFrameObject.clear();
    m_previewPaneObject.clear();
    m_renderInfoPaneObject.clear();
    m_footerPaneObject.clear();
    m_titleObject.clear();
    m_minimumSizeWarningObject.clear();

    if (screenWidth > 0 && screenHeight > 0)
    {
        m_outerFrameObject = ObjectFactory::makeFrame(
            screenWidth,
            screenHeight,
            Themes::Background,
            ObjectFactory::doubleLineBorder());
    }

    m_titleObject = ObjectFactory::makeTextUtf8(
        m_titleText,
        Themes::Subtitle);

    m_minimumSizeWarningObject = ObjectFactory::makeTextUtf8(
        m_minimumSizeMessage,
        Themes::Warning);

    m_titleX = 4;
    m_titleY = 0;

    m_warningX = 1;
    m_warningY = 1;
}

bool Donut3DScreen::isBelowMinimumScreenSize(int screenWidth, int screenHeight) const
{
    return screenWidth < MinimumScreenWidth || screenHeight < MinimumScreenHeight;
}
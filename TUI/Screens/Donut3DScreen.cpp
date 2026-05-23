#include "Screens/Donut3DScreen.h"

#include <string>
#include <sstream>
#include <iomanip>

#include "Core/Rect.h"
#include "Rendering/Composition/Alignment.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Objects/TextObjectFactory.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Surface.h"
#include "UI/Content/ComposedWindowContent.h"
#include "UI/Content/ImmediateModeWindowContent.h"
#include "UI/Content/EffectReferenceWindowContent.h"

using namespace UI;

namespace
{
    using Composition::PageComposer;

    constexpr int MinimumScreenWidth = 30;
    constexpr int MinimumScreenHeight = 12;

    template<typename T>
    std::string toFixedString(T value, int precision = 2)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision)
            << value;
        return ss.str();
    }
}

namespace DonutColors
{
    inline const Style Background =
        style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style BorderColor =
        style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style unfocusedWindow =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style unfocusedTitle =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style DonutWindow =
        style::Fg(Color::FromBasic(Color::Basic::Magenta))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style DonutTitle =
        style::Fg(Color::FromBasic(Color::Basic::BrightRed))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style InfoLabel =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style InfoValue =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style InfoAccent =
        style::Fg(Color::FromBasic(Color::Basic::Magenta))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style InfoDim =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
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
    m_windowManager.setDockTree(&m_dockTree);

    DonutEffectOptions options = m_Donut3D.getOptions();

    options.rotationXDegrees = 90.00f;
    options.rotationYDegrees = 0.00f;
    options.rotationZDegrees = 0.00f;

    options.rotationXSpeed = 0.001f;
    options.rotationYSpeed = 1.00f;
    options.rotationZSpeed = 0.50f;

    m_Donut3D.setOptions(options);
}

void Donut3DScreen::onExit()
{
    m_Donut3D.onExit();
}

bool Donut3DScreen::handleEvent(const Input::Event& event)
{
    return m_windowManager.handleEvent(event);
}

void Donut3DScreen::update(const Animation::TickEvent& event)
{
    m_windowManager.update(event);
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
    page.createInsetRegion("Safe", "Screen", 2, 2, 2, 1);

    const Rect safe = page.resolveRegion("Safe");
    const auto [footer, body] = page.splitBottom(safe, 5);
    const auto [rightPane, leftPane] = page.splitRight(body, body.size.width / 3);

    page.createRegion("LeftPane", leftPane);
    page.createRegion("RightPane", rightPane);
    page.createRegion("Footer", footer);

    page.createInsetRegion("3dViewPort", "LeftPane", 2, 1, 2, 1);
    page.createInsetRegion("FooterContent", "Footer", 2, 1, 2, 1);

    page.writeObjectInRegion(m_outerFrameObject, "Screen", Composition::Align::center());

    const Rect windowRectArr[2] =
    {
        page.resolveRegion("LeftPane"),
        page.resolveRegion("RightPane")
    };

    ensureLayout(safe, windowRectArr);

    page.drawPanel("Footer", DonutColors::Background, DonutColors::BorderColor, ObjectFactory::roundedBorder());
    page.drawNameValue("FooterContent", 0, "Palette", "Cyber Neon", DonutColors::InfoLabel, DonutColors::InfoValue, 2);

    m_titleObject.draw(buffer, m_titleX, m_titleY);

    m_windowManager.draw(surface);
}

void Donut3DScreen::ensureLayout(const Rect& viewport, const Rect* windowRectArr)
{
    if (m_layoutInitialized &&
        viewport.size.width == m_screenWidth &&
        viewport.size.height == m_screenHeight)
    {
        return;
    }

    m_screenWidth = viewport.size.width;
    m_screenHeight = viewport.size.height;

    m_windowManager.clear();

    auto previewContent =
        std::make_unique<UI::EffectReferenceWindowContent>(m_Donut3D);

    auto renderInfoContent =
        std::make_unique<UI::ImmediateModeWindowContent>(
            [this](Surface& surface, const Rect& bounds)
            {
                DonutEffectOptions options = m_Donut3D.getOptions();

                Composition::PageComposer composer(surface.buffer());

                composer.createRegion("Content", bounds);

                composer.drawSectionHeader(
                    "Content",
                    1,
                    "Render Pipeline",
                    DonutColors::DonutTitle,
                    DonutColors::InfoDim,
                    U'─');

                composer.drawNameValue(
                    "Content",
                    3,
                    "Host:   ",
                    "ContentWindow",
                    DonutColors::InfoLabel,
                    DonutColors::InfoValue,
                    2);

                composer.drawNameValue(
                    "Content",
                    4,
                    "Preview:",
                    "EffectReferenceWindowContent",
                    DonutColors::InfoLabel,
                    DonutColors::InfoValue,
                    2);

                composer.drawNameValue(
                    "Content",
                    5,
                    "Info:   ",
                    "ImmediateModeWindowContent",
                    DonutColors::InfoLabel,
                    DonutColors::InfoValue,
                    2);

                composer.drawHorizontalRule(
                    "Content",
                    7,
                    DonutColors::InfoDim,
                    U'─');

                composer.drawNameValue(
                    "Content",
                    9,
                    "Effect: ",
                    "Donut3D",
                    DonutColors::InfoLabel,
                    DonutColors::InfoValue,
                    2);

                composer.drawNameValue(
                    "Content",
                    10,
                    "Mode:   ",
                    "Classic ASCII",
                    DonutColors::InfoLabel,
                    DonutColors::InfoValue,
                    2);

                composer.drawNameValue(
                    "Content",
                    13,
                    "Elapsed Seconds:",
                    toFixedString(options.elapsedSeconds),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    14,
                    "X Rotation:",
                    toFixedString(options.rotationXDegrees),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    15,
                    "Y Rotation:",
                    toFixedString(options.rotationYDegrees),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    16,
                    "Z Rotation:",
                    toFixedString(options.rotationZDegrees),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    17,
                    "X Rotation Speed:",
                    toFixedString(options.rotationXSpeed),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    18,
                    "Y Rotation Speed:",
                    toFixedString(options.rotationYSpeed),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    19,
                    "Z Rotation Speed:",
                    toFixedString(options.rotationZSpeed),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    20,
                    "Camera Distance:",
                    toFixedString(options.cameraDistance),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    21,
                    "Object Scale: ",
                    toFixedString(options.scale),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    22,
                    "Theta Spacing:",
                    toFixedString(options.thetaSpacing),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawNameValue(
                    "Content",
                    23,
                    "Phi Spacing:  ",
                    toFixedString(options.phiSpacing),
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    2);

                composer.drawMeter(
                    "Content",
                    25,
                    "X Rotation",
                    options.rotationXDegrees,
                    360,
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    DonutColors::InfoDim,
                    14);

                composer.drawMeter(
                    "Content",
                    27,
                    "Y Rotation",
                    options.rotationYDegrees,
                    360,
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    DonutColors::InfoDim,
                    14);

                composer.drawMeter(
                    "Content",
                    29,
                    "Z Rotation",
                    options.rotationZDegrees,
                    360,
                    DonutColors::InfoLabel,
                    DonutColors::InfoAccent,
                    DonutColors::InfoDim,
                    14);
            });

    m_previewWindow = std::make_unique<UI::ContentWindow>(
        windowRectArr[0],
        m_previewWindowTitle,
        std::move(previewContent));

    m_renderInfoWindow = std::make_unique<UI::ContentWindow>(
        windowRectArr[1],
        m_renderInfoWindowTitle,
        std::move(renderInfoContent));

    m_previewWindow->setBorderGlyphs(ObjectFactory::roundedBorder());
    m_previewWindow->setBorderStyle(DonutColors::unfocusedWindow);
    m_previewWindow->setTitleStyle(DonutColors::unfocusedWindow);
    m_previewWindow->setHoveredBorderStyle(DonutColors::DonutWindow);
    m_previewWindow->setHoveredTitleStyle(DonutColors::DonutTitle);

    m_renderInfoWindow->setBorderGlyphs(ObjectFactory::roundedBorder());
    m_renderInfoWindow->setBorderStyle(DonutColors::unfocusedWindow);
    m_renderInfoWindow->setTitleStyle(DonutColors::unfocusedWindow);
    m_renderInfoWindow->setHoveredBorderStyle(DonutColors::DonutWindow);
    m_renderInfoWindow->setHoveredTitleStyle(DonutColors::DonutTitle);

    m_windowManager.addWindow(*m_previewWindow);
    m_windowManager.addWindow(*m_renderInfoWindow);

    ensureDockContentModel(viewport);
    m_layoutInitialized = true;
}

void Donut3DScreen::ensureDockContentModel(const Rect& viewport)
{
    if (viewport.size.width <= 0 || viewport.size.height <= 0)
    {
        return;
    }

    // DockTree represents actual docked relationships only.
    // The demo windows begin as independent floating windows, so do not
    // preload them into the tree here. Side-dock transactions build the
    // tree on mouse release from the real target and dragged windows.
    m_dockTree.clear();
    m_dockTree.setBounds(viewport);
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
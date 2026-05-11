#include "Screens/WindowDemoScreen.h"

#include <memory>
#include <utility>

#include "Core/Rect.h"
#include "Rendering/Surface.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Composition/Alignment.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Objects/TextObjectFactory.h"

namespace DemoColors
{
    inline const Style HeaderSurface =
          style::Fg(Color::FromBasic(Color::Basic::Black))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    inline const Style FooterSurface =
          style::Fg(Color::FromBasic(Color::Basic::Blue))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style Subtitle =
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style Background =
        style::Fg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::White),
            Color::FromIndexed(250),
            Color::FromRgb(230, 230, 230)))
        + style::Bg(ThemeColor::Basic256Rgb(
            Color::FromBasic(Color::Basic::Black),
            Color::FromIndexed(16),
            Color::FromRgb(12, 12, 12)));

    inline const Style BorderColor =
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style Warning =
          style::SlowBlink
        + style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Yellow));

    inline const Style unfocusedWindow =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style unfocusedTitle =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style DigiRainWindow =
          style::Fg(Color::FromBasic(Color::Basic::Green))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style DigiRainTitle =
          style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style DonutWindow =
        style::Fg(Color::FromBasic(Color::Basic::Magenta))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style DonutTitle =
        style::Fg(Color::FromBasic(Color::Basic::BrightRed))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style FireWindow =
          style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style FireTitle =
          style::Fg(Color::FromBasic(Color::Basic::Yellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style WaterWindow =
          style::Fg(Color::FromBasic(Color::Basic::Blue))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    inline const Style WaterTitle =
          style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Black));
}

namespace
{
    constexpr const char* DigitalRainTitle = "( Digital Rain Effect )";
    constexpr const char* DonutTitle = "( 3D Donut Effect )";
    constexpr const char* FireTitle = "( Fire Effect )";
    constexpr const char* WaterTitle = "( Water Wave Effect )";

    using Composition::Alignment;
    using Composition::PageComposer;

    constexpr int MinimumSceneWidth = 100;
    constexpr int MinimumSceneHeight = 30;

    bool isTooSmall(const ScreenBuffer& buffer)
    {
        return buffer.getWidth() < MinimumSceneWidth || buffer.getHeight() < MinimumSceneHeight;
    }

    void drawTooSmallMessage(ScreenBuffer& buffer)
    {
        buffer.clear(style::Bg(Color::FromBasic(Color::Basic::Black)));
        buffer.writeString(
            2,
            3,
            "This demo needs a larger terminal window.",
            DemoColors::Warning);
        buffer.writeString(
            2,
            5,
            "Recommended minimum: 100 x 30",
            DemoColors::Subtitle);
    }
}

WindowDemo::WindowDemo()
{
}

WindowDemo::~WindowDemo()
{
}

void WindowDemo::onEnter()
{
    m_fire.onEnter();
    m_digiRain.onEnter();
    m_donut.onEnter();
    m_water.onEnter();

    DigitalRainEffectOptions DigiRainOptions;
    DigiRainOptions.maxStreams = 60;
    DigiRainOptions.deadGlyphsSpawnRate = 8;
    DigiRainOptions.deadGlyphBlinkRate = 60;
    DigiRainOptions.glyphPool = 
        std::u32string(
        U"アァカサタナハマヤャラワガザダバパ"
        U"イィキシチニヒミリヰギジヂビピ"
        U"ウゥクスツヌフムユュルグズブヅプ"
        U"エェケセテネヘメレヱゲゼデベペ"
        U"オォコソトノホモヨョロヲゴゾドボポヴッン"
        U"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" // standard numbers and letters
        U"ΑβϲδεφϑհιյΚλʍƞɸπθʀστυƔѡϰψȥ"           // greek alphabet
        U"♪♫⌘₿äü∄∃ƒ±£µℇ");

    m_digiRain.setOptions(DigiRainOptions);
    m_windowManager.setDockTree(&m_dockTree);
}

bool WindowDemo::handleEvent(const Input::Event& event)
{
    return m_windowManager.handleEvent(event);
}

void WindowDemo::onExit()
{
    m_fire.onExit();
    m_digiRain.onExit();
    m_donut.onExit();
    m_water.onExit();
}

void WindowDemo::update(const Animation::TickEvent& event)
{
    m_windowManager.update(event);
}

void WindowDemo::draw(Surface& surface)
{
    auto& buffer = surface.buffer();

    if (isTooSmall(buffer))
    {
        drawTooSmallMessage(buffer);
        return;
    }

    PageComposer page(buffer);
    page.clearRegions();
    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Safe", "Screen", 2, 1, 2, 1);

    const Rect safe = page.resolveRegion("Safe");
    const auto [header, afterHeader]     = page.splitTop(safe, 5);
    const auto [footer, pageBody]        = page.splitBottom(afterHeader, 3);
    const auto [topPane, bottomPane]     = page.splitTop(pageBody, pageBody.size.height / 2);
    const auto [topLeft, topRight]       = page.splitLeft(topPane, topPane.size.width / 2);
    const auto [bottomLeft, bottomRight] = page.splitLeft(bottomPane, bottomPane.size.width / 2);
    
    page.createRegion("Header", header);
    page.createRegion("Footer", footer);
    page.createRegion("TopLeft", topLeft);
    page.createRegion("TopRight", topRight);
    page.createRegion("BottomLeft", bottomLeft);
    page.createRegion("BottomRight", bottomRight);

    page.createInsetRegion("TopLeftWindow", "TopLeft", 1, 0, 1, 0);
    page.createInsetRegion("TopRightWindow", "TopRight", 1, 0, 1, 0);
    page.createInsetRegion("BottomLeftWindow", "BottomLeft", 1, 0, 1, 0);
    page.createInsetRegion("BottomRightWindow", "BottomRight", 1, 0, 1, 0);

    page.drawPanel("Header",
        DemoColors::FooterSurface,
        DemoColors::FooterSurface,
        ObjectFactory::doubleLineBorder());

    page.drawPanel("Footer",
        DemoColors::FooterSurface,
        DemoColors::FooterSurface,
        ObjectFactory::singleLineBorder());

    const Rect windowRectArr[4] =
    {
        page.resolveRegion("TopLeftWindow"),
        page.resolveRegion("TopRightWindow"),
        page.resolveRegion("BottomLeftWindow"),
        page.resolveRegion("BottomRightWindow")
    };


    ensureLayout(pageBody, windowRectArr);
    drawInstructions(surface, footer);

    m_windowManager.draw(surface);
}

void WindowDemo::ensureLayout(const Rect& viewport, const Rect* windowRectArr)
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

    m_digiRainWindow = std::make_unique<EffectWindow>(windowRectArr[0], DigitalRainTitle, m_digiRain);
    m_donutWindow = std::make_unique<EffectWindow>(windowRectArr[1], DonutTitle, m_donut);
    m_fireWindow = std::make_unique<EffectWindow>(windowRectArr[2], FireTitle, m_fire);
    m_waterWindow = std::make_unique<EffectWindow>(windowRectArr[3], WaterTitle, m_water);

    m_digiRainWindow->setBorderGlyphs(ObjectFactory::roundedBorder());
    m_digiRainWindow->setBorderStyle(DemoColors::unfocusedWindow);
    m_digiRainWindow->setTitleStyle(DemoColors::unfocusedWindow);
    m_digiRainWindow->setHoveredBorderStyle(DemoColors::DigiRainWindow);
    m_digiRainWindow->setHoveredTitleStyle(DemoColors::DigiRainTitle);

    m_donutWindow->setBorderGlyphs(ObjectFactory::roundedBorder());
    m_donutWindow->setBorderStyle(DemoColors::unfocusedWindow);
    m_donutWindow->setTitleStyle(DemoColors::unfocusedWindow);
    m_donutWindow->setHoveredBorderStyle(DemoColors::DonutWindow);
    m_donutWindow->setHoveredTitleStyle(DemoColors::DonutTitle);

    m_fireWindow->setBorderGlyphs(ObjectFactory::roundedBorder());
    m_fireWindow->setBorderStyle(DemoColors::unfocusedWindow);
    m_fireWindow->setTitleStyle(DemoColors::unfocusedWindow);
    m_fireWindow->setHoveredBorderStyle(DemoColors::FireWindow);
    m_fireWindow->setHoveredTitleStyle(DemoColors::FireTitle);

    m_waterWindow->setBorderGlyphs(ObjectFactory::roundedBorder());
    m_waterWindow->setBorderStyle(DemoColors::unfocusedWindow);
    m_waterWindow->setTitleStyle(DemoColors::unfocusedWindow);
    m_waterWindow->setHoveredBorderStyle(DemoColors::WaterWindow);
    m_waterWindow->setHoveredTitleStyle(DemoColors::WaterTitle);

    m_windowManager.addWindow(*m_digiRainWindow.get());
    m_windowManager.addWindow(*m_donutWindow.get());
    m_windowManager.addWindow(*m_fireWindow.get());
    m_windowManager.addWindow(*m_waterWindow.get());

    ensureDockContentModel(viewport);
    m_layoutInitialized = true;
}

void WindowDemo::ensureDockContentModel(const Rect& viewport)
{
    if (viewport.size.width <= 0 || viewport.size.height <= 0)
    {
        return;
    }

    m_dockTree.setBounds(viewport);
    m_dockTree.clear();

    UI::DockContentDescriptor digiRainContent;
    digiRainContent.contentId = DigitalRainTitle;
    digiRainContent.title = DigitalRainTitle;

    UI::DockContentDescriptor donutContent;
    donutContent.contentId = DonutTitle;
    donutContent.title = DonutTitle;

    UI::DockContentDescriptor fireContent;
    fireContent.contentId = FireTitle;
    fireContent.title = FireTitle;

    UI::DockContentDescriptor waterContent;
    waterContent.contentId = WaterTitle;
    waterContent.title = WaterTitle;

    const int rootNodeId = m_dockTree.attachRoot(std::move(digiRainContent));

    m_dockTree.splitNode(
        rootNodeId,
        UI::DockSplitOrientation::Horizontal,
        0.5f,
        std::move(donutContent),
        false);

    if (UI::DockNode* digiRainNode = m_dockTree.findContent(DigitalRainTitle))
    {
        m_dockTree.splitNode(
            digiRainNode->id(),
            UI::DockSplitOrientation::Vertical,
            0.5f,
            std::move(fireContent),
            false);
    }

    if (UI::DockNode* donutNode = m_dockTree.findContent(DonutTitle))
    {
        m_dockTree.splitNode(
            donutNode->id(),
            UI::DockSplitOrientation::Vertical,
            0.5f,
            std::move(waterContent),
            false);
    }
}

void WindowDemo::drawInstructions(Surface& surface, const Rect& footer)
{
    ScreenBuffer& buffer = surface.buffer();

    const int x = footer.position.x + 2;
    const int y = footer.position.y + 1;

    buffer.writeString(
        x,
        y,
        "Drag windows normally. Hold Ctrl while dragging over another window to preview docking. Release over top/bottom/left/right/center.",
        DemoColors::Subtitle);
}
#include "Screens/WindowDemo.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Core/Rect.h"
#include "Rendering/Compositor.h"
#include "Rendering/Composition/Alignment.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/AppThemes.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Styles/UIThemes.h"
#include "Rendering/Surface.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Objects/TextObjectFactory.h"

namespace
{
    using Composition::Alignment;
    using Composition::PageComposer;

    Style makeWindowStyle(Color::Basic fg, Color::Basic bg, bool bold = false)
    {
        Style result = style::Fg(Color::FromBasic(fg)) + style::Bg(Color::FromBasic(bg));

        if (bold)
        {
            result = result + style::Bold;
        }

        return result;
    }

    const Style BackdropStyle =
        makeWindowStyle(Color::Basic::Cyan, Color::Basic::Black);

    const Style BackdropText =
        makeWindowStyle(Color::Basic::BrightYellow, Color::Basic::Black);

    const Style LeftWindowFill =
        makeWindowStyle(Color::Basic::BrightWhite, Color::Basic::Blue);

    const Style RightWindowFill =
        makeWindowStyle(Color::Basic::BrightWhite, Color::Basic::Magenta);

    const Style StatusWindowFill =
        makeWindowStyle(Color::Basic::BrightGreen, Color::Basic::Black);

    const Style ModalFill =
        makeWindowStyle(Color::Basic::BrightYellow, Color::Basic::Red, true);

    const Style ActiveFrame =
        makeWindowStyle(Color::Basic::BrightCyan, Color::Basic::Black, true);

    const Style ModalFrame =
        makeWindowStyle(Color::Basic::BrightYellow, Color::Basic::Black, true);

    void writeLines(
        ScreenBuffer& buffer,
        const Rect& bounds,
        const std::vector<std::string>& lines,
        const Style& style)
    {
        const int maxLines = std::max(0, bounds.size.height);

        for (int index = 0; index < static_cast<int>(lines.size()) && index < maxLines; ++index)
        {
            buffer.writeString(
                bounds.position.x,
                bounds.position.y + index,
                lines[static_cast<std::size_t>(index)],
                style);
        }
    }
}

WindowDemo::DemoWindow::DemoWindow(
    const Rect& bounds,
    std::string title)
    : Window(bounds, std::move(title))
{
}

void WindowDemo::DemoWindow::setLines(std::vector<std::string> lines)
{
    m_lines = std::move(lines);
}

void WindowDemo::DemoWindow::draw(Surface& surface)
{
    Window::draw(surface);
    writeLines(surface.buffer(), contentBounds(), m_lines, backgroundStyle());
}

WindowDemo::DemoPopup::DemoPopup(
    const Rect& bounds,
    std::string title)
    : PopupWindow(bounds, std::move(title), true)
{
}

void WindowDemo::DemoPopup::setLines(std::vector<std::string> lines)
{
    m_lines = std::move(lines);
}

void WindowDemo::DemoPopup::draw(Surface& surface)
{
    PopupWindow::draw(surface);
    writeLines(surface.buffer(), contentBounds(), m_lines, backgroundStyle());
}

WindowDemo::WindowDemo()
    : m_leftWindow(Rect{ Point{ 4, 4 }, Size{ 36, 12 } }, "Base Window")
    , m_rightWindow(Rect{ Point{ 24, 8 }, Size{ 40, 12 } }, "Front Window")
    , m_statusWindow(Rect{ Point{ 8, 22 }, Size{ 64, 7 } }, "WindowManager Status")
    , m_modalPopup(Rect{ Point{ 18, 7 }, Size{ 46, 11 } }, "Modal PopupWindow")
{
    configureWindows();
}

void WindowDemo::onEnter()
{
    m_elapsedSeconds = 0.0;
    m_previousStage = -1;

    m_windowManager.clear();

    m_windowManager.addWindow(m_leftWindow);
    m_windowManager.addWindow(m_rightWindow);
    m_windowManager.addWindow(m_statusWindow);
    m_windowManager.addWindow(m_modalPopup);

    m_windowManager.setZOrder(m_leftWindow, 10);
    m_windowManager.setZOrder(m_rightWindow, 20);
    m_windowManager.setZOrder(m_statusWindow, 30);
    m_windowManager.setZOrder(m_modalPopup, 100);

    m_leftWindow.show();
    m_rightWindow.show();
    m_statusWindow.show();
    m_modalPopup.open();
}

bool WindowDemo::handleEvent(const Input::Event& event)
{
    return m_windowManager.handleEvent(event);
}

void WindowDemo::update(double deltaTime)
{
    m_elapsedSeconds += deltaTime;

    const int stage = static_cast<int>(std::floor(m_elapsedSeconds / 4.0)) % 4;

    if (stage == m_previousStage)
    {
        return;
    }

    m_previousStage = stage;

    switch (stage)
    {
    case 0:
        m_leftWindow.show();
        m_rightWindow.show();
        m_modalPopup.open();
        m_windowManager.setZOrder(m_leftWindow, 10);
        m_windowManager.setZOrder(m_rightWindow, 20);
        m_windowManager.setZOrder(m_statusWindow, 30);
        m_windowManager.setZOrder(m_modalPopup, 100);
        break;

    case 1:
        m_modalPopup.close();
        m_windowManager.bringToFront(m_leftWindow);
        break;

    case 2:
        m_rightWindow.hide();
        m_windowManager.bringToFront(m_statusWindow);
        break;

    case 3:
        m_rightWindow.show();
        m_windowManager.bringToFront(m_rightWindow);
        m_modalPopup.open();
        m_windowManager.bringToFront(m_modalPopup);
        break;
    }

    m_windowManager.update(deltaTime);
}

void WindowDemo::draw(Surface& surface)
{
    PageComposer page(surface.buffer());
    page.clearRegions();

    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Safe", "Screen", 2, 1, 2, 1);

    const Rect screen = page.resolveRegion("Screen");
    const auto safe = page.resolveRegion("Safe");
    const auto [explainer, body] = page.splitTop(safe, 6);

    page.createRegion("Explainer", explainer);
    page.createInsetRegion("ExplainerSafe", "Explainer", 2, 3, 2, 1);

    TextObject mainWindowFill = ObjectFactory::makePatternFill(
        screen.size.width,
        screen.size.height,
        ObjectFactory::bubblesPattern(),
        BackdropStyle);

    page.writeObjectInRegion(mainWindowFill, "Screen", Composition::Align::topCenter());

    TextObject explainerBox = ObjectFactory::makeBorderedBox(
        explainer.size.width,
        explainer.size.height,
        U' ',
        BackdropStyle,
        ObjectFactory::doubleLineBorder());

    page.writeObjectInRegion(explainerBox,
        "Explainer",
        Composition::Align::center());

    page.writePanelTitle("Explainer", "Explainer Object", BackdropText);

    page.writeWrappedTextInRegion(
        "Phase 8 Integration Demo — LayerInstance + Compositor + Panel + Window + PopupWindow + WindowManager\nThe scene cycles every 4 seconds through modal, z-order, and visibility states.",
        "ExplainerSafe",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center });

    layoutWindows(surface);

    std::vector<LayerInstance> layers = m_windowManager.buildLayers();

    Compositor::compose(
        layers,
        surface,
        Composition::WritePresets::solidObject());
}

void WindowDemo::configureWindows()
{
    m_leftWindow.setBackgroundStyle(LeftWindowFill);
    m_leftWindow.setBorderStyle(ActiveFrame);
    m_leftWindow.setBorderGlyphs(ObjectFactory::roundedBorder());
    m_leftWindow.setTitleStyle(ActiveFrame);
    m_leftWindow.setLines({
        "stage 1: z-order: starts behind",
        "stage 2: brought forward",
        "",
        "This proves windows can",
        "overlap without changing",
        "their own draw code."
        });

    m_rightWindow.setBackgroundStyle(RightWindowFill);
    m_rightWindow.setBorderStyle(ActiveFrame);
    m_rightWindow.setBorderGlyphs(ObjectFactory::roundedBorder());
    m_rightWindow.setTitleStyle(ActiveFrame);
    m_rightWindow.setLines({
        "stage 1: z-order: starts in front",
        "stage 2: hidden",
        "stage 3: restored",
        "",
        "Visibility changes are",
        "managed by WindowManager."
        });

    m_statusWindow.setBackgroundStyle(StatusWindowFill);
    m_statusWindow.setBorderStyle(UIThemes::Border);
    m_statusWindow.setBorderGlyphs(ObjectFactory::roundedBorder());
    m_statusWindow.setTitleStyle(UIThemes::PanelTitle);
    m_statusWindow.setLines({
        "Validation targets:",
        "- overlapping windows",
        "- deterministic z-order",
        "- show/hide behavior",
        "- modal routing priority",
        "- compositor-based layering"
        });

    m_modalPopup.setBackgroundStyle(ModalFill);
    m_modalPopup.setBorderStyle(ModalFrame);
    m_modalPopup.setBorderGlyphs(ObjectFactory::roundedBorder());
    m_modalPopup.setTitleStyle(ModalFrame);
    m_modalPopup.setLines({
        "MODAL ACTIVE",
        "",
        "This popup is topmost and modal.",
        "Lower windows still render underneath,",
        "but WindowManager::canRouteTo()",
        "blocks lower-window interaction."
        });
}

void WindowDemo::layoutWindows(const Surface& surface)
{
    const int width = surface.buffer().getWidth();
    const int height = surface.buffer().getHeight();

    const int leftWidth = width / 2;
    const int rightWidth = width / 2;
    const int windowHeight = height / 2;

    m_leftWindow.setBounds(Rect{
        Point{ 4, 8 },
        Size{ leftWidth, windowHeight }
        });

    m_rightWindow.setBounds(Rect{
        Point{ std::max(8, width / 2 - 10), 11 },
        Size{ rightWidth, windowHeight }
        });

    m_statusWindow.setBounds(Rect{
        Point{ 6, std::max(17, height - 10) },
        Size{ std::min(width - 12, 72), 7 }
        });

    m_modalPopup.setBounds(Rect{
        Point{ std::max(2, width / 2 - 23), std::max(3, height / 2 - 6) },
        Size{ std::min(46, width - 4), 11 }
        });
}
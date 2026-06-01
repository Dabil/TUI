#include "Screens/ScrollableTileGridDemoScreen.h"

#include <algorithm>
#include <string>

#include "Core/Rect.h"
#include "Input/Command.h"
#include "Input/Event.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/AppThemes.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/UIThemes.h"
#include "Rendering/Surface.h"

namespace
{
    constexpr int MinimumWidth = 70;
    constexpr int MinimumHeight = 22;

    constexpr int DemoGridWidth = 120;
    constexpr int DemoGridHeight = 60;

    const Style ScreenStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style HeaderStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    const Style PanelStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style BorderStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style TitleStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style FloorStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style GridLineStyle =
        style::Fg(Color::FromBasic(Color::Basic::Blue))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style WallStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style WaterStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightCyan))
        + style::Bg(Color::FromBasic(Color::Basic::Blue));

    const Style ForestStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightGreen))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style RoadStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightYellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style MarkerStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightRed))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    std::string scrollPositionText(const TileGridView& view)
    {
        const Viewport& viewport = view.viewport();

        return "Scroll "
            + std::to_string(viewport.scrollX())
            + ","
            + std::to_string(viewport.scrollY())
            + " / max "
            + std::to_string(viewport.maxScrollX())
            + ","
            + std::to_string(viewport.maxScrollY());
    }
}

ScrollableTileGridDemoScreen::ScrollableTileGridDemoScreen()
    : m_gridView("Scrollable Tile/Grid View")
{
    configureControls();
    buildDemoGrid();
}

void ScrollableTileGridDemoScreen::onEnter()
{
    m_gridView.home();
    m_lastAction = "Demo grid reset to origin.";
    updateStatusBar();
}

bool ScrollableTileGridDemoScreen::handleEvent(const Input::Event& event)
{
    if (const Input::CommandEvent* commandEvent = event.asCommand())
    {
        return handleCommand(commandEvent->command);
    }

    const Point before = m_gridView.viewport().scrollOffset();

    if (m_gridView.handleEvent(event))
    {
        const Point after = m_gridView.viewport().scrollOffset();

        if (before.x != after.x || before.y != after.y)
        {
            m_lastAction = "Mouse wheel scrolled grid.";
        }

        return true;
    }

    return false;
}

void ScrollableTileGridDemoScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    if (buffer.getWidth() < MinimumWidth || buffer.getHeight() < MinimumHeight)
    {
        drawTooSmall(surface);
        return;
    }

    buffer.clear(ScreenStyle);

    updateLayout(surface);
    updateStatusBar();

    m_headerPanel.draw(surface);
    drawHeader(surface);

    m_gridView.draw(surface);
    m_statusBar.draw(surface);
}

void ScrollableTileGridDemoScreen::configureControls()
{
    m_headerPanel.setTitle("Phase 10 Demo");
    m_headerPanel.setBackgroundStyle(HeaderStyle);
    m_headerPanel.setBorderStyle(HeaderStyle);
    m_headerPanel.setTitleStyle(HeaderStyle);

    m_gridView.setBackgroundStyle(PanelStyle);
    m_gridView.setBorderStyle(BorderStyle);
    m_gridView.setTitleStyle(TitleStyle);
    m_gridView.setEmptyCellStyle(PanelStyle);

    m_statusBar.setInstructions("Scrollable grid/tile demo");
    m_statusBar.setCommandHints({
        "Arrows: scroll",
        "PgUp/PgDn: page",
        "Home: origin",
        "End: bottom-right",
        "Enter/Right: next screen"
        });
}

void ScrollableTileGridDemoScreen::buildDemoGrid()
{
    TileGrid grid(DemoGridWidth, DemoGridHeight);

    for (int y = 0; y < DemoGridHeight; ++y)
    {
        for (int x = 0; x < DemoGridWidth; ++x)
        {
            char32_t glyph = U'.';
            Style style = FloorStyle;

            if (x == 0 || y == 0 || x == DemoGridWidth - 1 || y == DemoGridHeight - 1)
            {
                glyph = U'#';
                style = WallStyle;
            }
            else if (x % 10 == 0 || y % 6 == 0)
            {
                glyph = U'+';
                style = GridLineStyle;
            }
            else if ((x > 12 && x < 35 && y > 10 && y < 18)
                || (x > 78 && x < 108 && y > 36 && y < 47))
            {
                glyph = U'~';
                style = WaterStyle;
            }
            else if ((x + y) % 17 == 0 || (x * 3 + y) % 29 == 0)
            {
                glyph = U'^';
                style = ForestStyle;
            }
            else if (y == (x / 3) + 8 || y == 48 - (x / 4))
            {
                glyph = U'=';
                style = RoadStyle;
            }

            grid.set(x, y, makeCell(glyph, style));
        }
    }

    for (int y = 5; y < DemoGridHeight - 5; y += 10)
    {
        for (int x = 8; x < DemoGridWidth - 8; x += 18)
        {
            grid.set(x, y, makeCell(U'X', MarkerStyle));
        }
    }

    m_gridView.setGrid(std::move(grid));
}

void ScrollableTileGridDemoScreen::updateLayout(Surface& surface)
{
    const ScreenBuffer& buffer = surface.buffer();

    const int width = buffer.getWidth();
    const int height = buffer.getHeight();

    m_headerPanel.setBounds(Rect{
        Point{ 1, 1 },
        Size{ std::max(0, width - 2), 5 }
        });

    m_gridView.setBounds(Rect{
        Point{ 2, 7 },
        Size{ std::max(0, width - 4), std::max(0, height - 10) }
        });

    m_statusBar.setBounds(Rect{
        Point{ 0, std::max(0, height - 2) },
        Size{ width, 2 }
        });
}

void ScrollableTileGridDemoScreen::updateStatusBar()
{
    m_statusBar.setInstructions(m_lastAction + "  " + scrollPositionText(m_gridView));
}

bool ScrollableTileGridDemoScreen::handleCommand(const Input::Command& command)
{
    bool changed = false;

    switch (command.code)
    {
    case Input::CommandCode::MoveUp:
        changed = m_gridView.scrollUp();
        m_lastAction = changed ? "Scrolled up." : "Already at top edge.";
        return true;

    case Input::CommandCode::MoveDown:
        changed = m_gridView.scrollDown();
        m_lastAction = changed ? "Scrolled down." : "Already at bottom edge.";
        return true;

    case Input::CommandCode::MoveLeft:
        changed = m_gridView.scrollLeft();
        m_lastAction = changed ? "Scrolled left." : "Already at left edge.";
        return true;

    case Input::CommandCode::MoveRight:
        changed = m_gridView.scrollRight();
        m_lastAction = changed ? "Scrolled right." : "Already at right edge.";
        return true;

    case Input::CommandCode::PageUp:
        changed = m_gridView.pageUp();
        m_lastAction = changed ? "Paged up." : "Already at top page.";
        return true;

    case Input::CommandCode::PageDown:
        changed = m_gridView.pageDown();
        m_lastAction = changed ? "Paged down." : "Already at bottom page.";
        return true;

    case Input::CommandCode::MoveHome:
        changed = m_gridView.home();
        m_lastAction = changed ? "Moved to grid origin." : "Already at grid origin.";
        return true;

    case Input::CommandCode::MoveEnd:
    {
        const Point before = m_gridView.viewport().scrollOffset();

        m_gridView.viewport().scrollTo(
            m_gridView.viewport().maxScrollX(),
            m_gridView.viewport().maxScrollY());

        const Point after = m_gridView.viewport().scrollOffset();

        changed = before.x != after.x || before.y != after.y;
        m_lastAction = changed ? "Moved to bottom-right corner." : "Already at bottom-right corner.";
        return true;
    }

    default:
        return false;
    }
}

void ScrollableTileGridDemoScreen::drawHeader(Surface& surface) const
{
    ScreenBuffer& buffer = surface.buffer();
    const Rect content = m_headerPanel.contentBounds();

    if (content.size.width <= 0 || content.size.height <= 0)
    {
        return;
    }

    buffer.writeString(
        content.position.x,
        content.position.y,
        "Large generated tile grid rendered through TileGridView -> ScrollablePanel -> Surface.",
        HeaderStyle);

    if (content.size.height > 1)
    {
        buffer.writeString(
            content.position.x,
            content.position.y + 1,
            "Only the visible grid region is copied into the viewport surface each frame.",
            HeaderStyle);
    }

    if (content.size.height > 2)
    {
        buffer.writeString(
            content.position.x,
            content.position.y + 2,
            "Legend: # wall, . floor, + gridline, ~ water, ^ trees, = road, X markers.",
            HeaderStyle);
    }
}

void ScrollableTileGridDemoScreen::drawTooSmall(Surface& surface) const
{
    ScreenBuffer& buffer = surface.buffer();

    buffer.clear(ScreenStyle);
    buffer.writeString(
        1,
        1,
        "Terminal too small for Scrollable Tile/Grid Demo.",
        UIThemes::WarningText);

    buffer.writeString(
        1,
        3,
        "Please resize to at least "
        + std::to_string(MinimumWidth)
        + "x"
        + std::to_string(MinimumHeight)
        + ".",
        UIThemes::Label);
}

TileGrid::Cell ScrollableTileGridDemoScreen::makeCell(char32_t glyph, const Style& style)
{
    TileGrid::Cell cell;
    cell.glyph = glyph;
    cell.cluster.clear();
    cell.style = style;
    cell.kind = CellKind::Glyph;
    cell.width = CellWidth::One;
    return cell;
}
#include "Screens/ShowcaseScreen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Assets/Loaders/AsciiBannerFontLoader.h"
#include "Assets/Models/AsciiBannerFont.h"
#include "Core/Rect.h"
#include "Rendering/Composition/Alignment.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Objects/AsciiBanner.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Objects/TextObjectFactory.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Surface.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Styles/ThemeColor.h"

namespace
{
    using Composition::Alignment;
    using Composition::PageComposer;
    using Composition::WritePresets::solidObject;
    using Composition::WritePresets::visibleObject;
    using Composition::WritePresets::authoredObject;

    constexpr int MinimumSceneWidth = 84;
    constexpr int MinimumSceneHeight = 28;

    ThemeColor makeThemeColor(
        Color::Basic basic,
        std::uint8_t indexed,
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue)
    {
        return ThemeColor::Basic256Rgb(
            Color::FromBasic(basic),
            Color::FromIndexed(indexed),
            Color::FromRgb(red, green, blue));
    }

    Style makeStyle(
        const ThemeColor& fg,
        const ThemeColor& bg,
        bool bold = false,
        bool dim = false,
        bool underline = false)
    {
        Style result = style::Fg(fg) + style::Bg(bg);

        if (bold)
        {
            result = result + style::Bold;
        }

        if (dim)
        {
            result = result + style::Dim;
        }

        if (underline)
        {
            result = result + style::Underline;
        }

        return result;
    }

    const Style ControlSurface = makeStyle(
        makeThemeColor(Color::Basic::BrightWhite, 255, 240, 247, 255),
        makeThemeColor(Color::Basic::Black, 17, 7, 14, 28));

    const Style ControlPanel = makeStyle(
        makeThemeColor(Color::Basic::BrightWhite, 255, 242, 247, 255),
        makeThemeColor(Color::Basic::Black, 18, 13, 25, 46));

    const Style ControlFrame = makeStyle(
        makeThemeColor(Color::Basic::BrightCyan, 81, 104, 214, 255),
        makeThemeColor(Color::Basic::Black, 18, 13, 25, 46),
        true);

    const Style ControlAccent = makeStyle(
        makeThemeColor(Color::Basic::BrightYellow, 228, 255, 218, 92),
        makeThemeColor(Color::Basic::Black, 18, 13, 25, 46),
        true);

    const Style ControlInfo = makeStyle(
        makeThemeColor(Color::Basic::BrightCyan, 117, 112, 235, 255),
        makeThemeColor(Color::Basic::Black, 18, 13, 25, 46),
        true);

    const Style TerminalHeaderSurface = makeStyle(
        makeThemeColor(Color::Basic::Black, 0, 0, 10, 0),
        makeThemeColor(Color::Basic::Green, 34, 45, 140, 62),
        true);

    const Style TerminalSurface = makeStyle(
        makeThemeColor(Color::Basic::BrightGreen, 46, 126, 255, 168),
        makeThemeColor(Color::Basic::Black, 16, 0, 64, 0),
        true);

    const Style TerminalPanel = makeStyle(
        makeThemeColor(Color::Basic::BrightGreen, 46, 126, 255, 168),
        makeThemeColor(Color::Basic::Black, 22, 4, 18, 6));

    const Style TerminalFrame = makeStyle(
        makeThemeColor(Color::Basic::BrightGreen, 84, 76, 255, 122),
        makeThemeColor(Color::Basic::Black, 22, 4, 18, 6),
        true);

    const Style TerminalInverse = makeStyle(
        makeThemeColor(Color::Basic::Black, 16, 0, 10, 0),
        makeThemeColor(Color::Basic::Green, 34, 45, 140, 62),
        true);

    const Style NeonSurface = makeStyle(
        makeThemeColor(Color::Basic::BrightWhite, 255, 255, 244, 255),
        makeThemeColor(Color::Basic::Black, 17, 14, 8, 26));

    const Style NeonDialogFill = makeStyle(
        makeThemeColor(Color::Basic::BrightWhite, 255, 255, 245, 255),
        makeThemeColor(Color::Basic::Magenta, 91, 63, 22, 84));

    const Style NeonFrame = makeStyle(
        makeThemeColor(Color::Basic::BrightMagenta, 213, 255, 86, 214),
        makeThemeColor(Color::Basic::Magenta, 91, 63, 22, 84),
        true);

    const Style NeonAccent = makeStyle(
        makeThemeColor(Color::Basic::BrightCyan, 117, 115, 248, 255),
        makeThemeColor(Color::Basic::Magenta, 91, 63, 22, 84),
        true);

    const Style NeonGold = makeStyle(
        makeThemeColor(Color::Basic::BrightYellow, 228, 255, 214, 105),
        makeThemeColor(Color::Basic::Magenta, 91, 63, 22, 84),
        true);

    const Style OpsSurface = makeStyle(
        makeThemeColor(Color::Basic::BrightWhite, 255, 240, 240, 240),
        makeThemeColor(Color::Basic::Black, 16, 7, 7, 9));

    const Style OpsPanel = makeStyle(
        makeThemeColor(Color::Basic::BrightWhite, 255, 246, 246, 246),
        makeThemeColor(Color::Basic::Black, 235, 24, 24, 28));

    const Style OpsFrame = makeStyle(
        makeThemeColor(Color::Basic::BrightRed, 203, 255, 118, 69),
        makeThemeColor(Color::Basic::Black, 235, 24, 24, 28),
        true);

    const Style OpsAmber = makeStyle(
        makeThemeColor(Color::Basic::BrightYellow, 220, 255, 191, 71),
        makeThemeColor(Color::Basic::Black, 235, 24, 24, 28),
        true);

    const Style OpsCyan = makeStyle(
        makeThemeColor(Color::Basic::BrightCyan, 117, 108, 229, 255),
        makeThemeColor(Color::Basic::Black, 235, 24, 24, 28),
        true);

    const Style MutedOverlay = makeStyle(
        makeThemeColor(Color::Basic::White, 245, 144, 156, 176),
        makeThemeColor(Color::Basic::Black, 16, 7, 14, 28),
        false,
        true);

    void fillRect(ScreenBuffer& buffer, const Rect& rect, const Style& style, char32_t glyph = U' ')
    {
        if (rect.size.width <= 0 || rect.size.height <= 0)
        {
            return;
        }

        buffer.fillRect(rect, glyph, style);
    }

    void fillVerticalGradient(
        ScreenBuffer& buffer,
        const Rect& rect,
        const ThemeColor& foreground,
        const std::array<Color::RgbValue, 2>& backgroundRange,
        Color::Basic basicFallback,
        std::uint8_t indexedBase)
    {
        if (rect.size.width <= 0 || rect.size.height <= 0)
        {
            return;
        }

        const int heightSpan = std::max(1, rect.size.height - 1);

        for (int y = 0; y < rect.size.height; ++y)
        {
            const double ratio = static_cast<double>(y) / static_cast<double>(heightSpan);

            const auto blend = [&](std::uint8_t a, std::uint8_t b) -> std::uint8_t
                {
                    return static_cast<std::uint8_t>(
                        std::round(static_cast<double>(a) + (static_cast<double>(b) - static_cast<double>(a)) * ratio));
                };

            const Style rowStyle = style::Fg(foreground)
                + style::Bg(ThemeColor::Basic256Rgb(
                    Color::FromBasic(basicFallback),
                    Color::FromIndexed(static_cast<std::uint8_t>(indexedBase + std::min(y, 10))),
                    Color::FromRgb(
                        blend(backgroundRange[0].red, backgroundRange[1].red),
                        blend(backgroundRange[0].green, backgroundRange[1].green),
                        blend(backgroundRange[0].blue, backgroundRange[1].blue))));

            buffer.fillRect(
                Rect{ Point{ rect.position.x, rect.position.y + y }, Size{ rect.size.width, 1 } },
                U' ',
                rowStyle);
        }
    }

    void drawFrame(ScreenBuffer& buffer, const Rect& rect, const Style& style, const ObjectFactory::BorderGlyphs& glyphs)
    {
        buffer.drawFrame(
            rect,
            style,
            glyphs.topLeft,
            glyphs.topRight,
            glyphs.bottomLeft,
            glyphs.bottomRight,
            glyphs.horizontal,
            glyphs.vertical);
    }

    std::string makeProgressBar(int width, double ratio, char fillGlyph, char emptyGlyph = '.')
    {
        ratio = std::clamp(ratio, 0.0, 1.0);
        const int filled = static_cast<int>(std::round(ratio * static_cast<double>(width)));

        std::string bar = "[";
        for (int i = 0; i < width; ++i)
        {
            bar += (i < filled) ? fillGlyph : emptyGlyph;
        }
        bar += "] ";
        bar += std::to_string(static_cast<int>(std::round(ratio * 100.0)));
        bar += "%";
        return bar;
    }

    std::string makeMarquee(std::string_view message, int width, int offset)
    {
        if (width <= 0)
        {
            return "";
        }

        std::string padded = "   " + std::string(message) + "   ";
        while (static_cast<int>(padded.size()) < (width * 3))
        {
            padded += std::string(message) + "   ";
        }

        offset %= static_cast<int>(padded.size());
        if (offset < 0)
        {
            offset += static_cast<int>(padded.size());
        }

        std::string result;
        result.reserve(static_cast<std::size_t>(width));
        for (int i = 0; i < width; ++i)
        {
            result.push_back(padded[static_cast<std::size_t>((offset + i) % static_cast<int>(padded.size()))]);
        }

        return result;
    }

    double pulse(double timeSeconds, double speed, double phase = 0.0)
    {
        return (std::sin((timeSeconds * speed) + phase) + 1.0) * 0.5;
    }

    const TextObject& waveObject()
    {
        static const TextObject object = TextObject::fromUtf8(
            "  _      __      _      __      _      __      _            \n"
            " / \\    /  \\    / \\    /  \\    / \\    /  \\    / \\    \n"
            "/   \\__/ /\\ \\__/   \\__/ /\\ \\__/   \\__/ /\\ \\__/   \\\n"
            "\\___/  \\ \\/ /  \\___/  \\ \\/ /  \\___/  \\ \\/ /  \\___/\n"
            "        \\__/           \\__/           \\__/                 ");
        return object;
    }

    const TextObject& verticalMeterObject()
    {
        static const TextObject object = TextObject::fromUtf8(
            "|-- 100\n"
            "|-     \n"
            "|-- 90 \n"
            "|-     \n"
            "|-- 80 \n"
            "|-     \n"
            "|-- 70 \n"
            "|-     \n"
            "|-- 60 \n"
            "|-     \n"
            "|-- 50 \n"
            "|-     \n"
            "|-- 40 \n"
            "|-     \n"
            "|-- 30 \n"
            "|-     \n"
            "|-- 20 \n"
            "|-     \n"
            "|-- 10 \n"
            "|-     \n"
            "|-- 0  ");
        return object;
    }

    const AsciiBannerFont* findOrLoadFont(const std::string& fileName)
    {
        static std::unordered_map<std::string, AsciiBannerFont> cache;

        const auto found = cache.find(fileName);
        if (found != cache.end())
        {
            return found->second.isLoaded() ? &found->second : nullptr;
        }

        const std::filesystem::path fontPath = std::filesystem::path("Assets") / "Fonts" / "FIGlet" / fileName;
        const AsciiBannerFontLoader::LoadResult loadResult =
            AsciiBannerFontLoader::loadFromFile(fontPath.string());

        cache.emplace(fileName, loadResult.font);

        const auto inserted = cache.find(fileName);
        if (inserted == cache.end() || !inserted->second.isLoaded())
        {
            return nullptr;
        }

        return &inserted->second;
    }

    TextObject makeBanner(
        const std::string& fileName,
        std::string_view text,
        const Style& style,
        AsciiBanner::ComposeMode composeMode = AsciiBanner::ComposeMode::Kern)
    {
        const AsciiBannerFont* font = findOrLoadFont(fileName);
        if (font == nullptr)
        {
            return TextObject::fromUtf8(text, style);
        }

        AsciiBanner::RenderOptions options;
        options.composeMode = composeMode;
        options.alignment = AsciiBanner::Alignment::Left;
        options.trimTrailingSpaces = true;
        options.replaceHardBlankWithSpace = true;
        options.transparentSpaces = true;

        return AsciiBanner::generateTextObject(*font, text, style, options);
    }

    bool isTooSmall(const ScreenBuffer& buffer)
    {
        return buffer.getWidth() < MinimumSceneWidth || buffer.getHeight() < MinimumSceneHeight;
    }

    void drawTooSmallMessage(ScreenBuffer& buffer, const std::string& sceneName, const Style& style)
    {
        buffer.clear(style::Bg(Color::FromBasic(Color::Basic::Black)));
        buffer.writeString(2, 1, sceneName, style);
        buffer.writeString(
            2,
            3,
            "This showcase scene needs a larger terminal window.",
            Themes::Warning);
        buffer.writeString(
            2,
            5,
            "Recommended minimum: 84 x 28",
            Themes::Subtitle);
    }

    Rect insetRect(const Rect& rect, int left, int top, int right, int bottom)
    {
        return Rect{
            Point{ rect.position.x + left, rect.position.y + top },
            Size{ std::max(0, rect.size.width - left - right), std::max(0, rect.size.height - top - bottom) }
        };
    }

    void paintPanel(
        ScreenBuffer& buffer,
        const Rect& rect,
        const Style& fillStyle,
        const Style& frameStyle,
        const ObjectFactory::BorderGlyphs& glyphs = ObjectFactory::singleLineBorder())
    {
        fillRect(buffer, rect, fillStyle);
        drawFrame(buffer, rect, frameStyle, glyphs);
    }

    Rect drawPanelTitleBar(
        ScreenBuffer& buffer,
        const Rect& rect,
        std::string_view title,
        const Style& titleStyle,
        const Style& barStyle)
    {
        const Rect titleBar{ rect.position, Size{ rect.size.width, std::min(3, rect.size.height) } };
        fillRect(buffer, titleBar, barStyle);
        buffer.writeString(titleBar.position.x + 2, titleBar.position.y + 1, std::string(title), titleStyle);
        return insetRect(rect, 2, titleBar.size.height + 1, 2, 1);
    }

    struct RetroTerminalComponent
    {
        TextObject shell;
        Rect localMonitorViewport;

        bool isLoaded() const
        {
            return shell.isLoaded();
        }

        Rect resolveShellRect(const Rect& hostRegion) const
        {
            if (!shell.isLoaded())
            {
                return Rect{ hostRegion.position, Size{ 0, 0 } };
            }

            return Rect{
                hostRegion.position,
                Size{ shell.getWidth(), shell.getHeight() }
            };
        }

        Rect resolveMonitorRect(const Rect& hostRegion) const
        {
            const Rect shellRect = resolveShellRect(hostRegion);

            return Rect{
                Point{
                    shellRect.position.x + localMonitorViewport.position.x,
                    shellRect.position.y + localMonitorViewport.position.y
                },
                localMonitorViewport.size
            };
        }

        void drawShell(
            ScreenBuffer& buffer,
            const Rect& hostRegion,
            const Composition::WritePolicy& policy = solidObject()) const
        {
            if (!shell.isLoaded())
            {
                return;
            }

            const Rect shellRect = resolveShellRect(hostRegion);
            shell.draw(buffer, shellRect.position.x, shellRect.position.y, policy);
        }

        void drawLogLines(
            ScreenBuffer& buffer,
            const Rect& hostRegion,
            const std::vector<std::string>& lines,
            int firstLineIndex,
            const Style& textStyle,
            const Style* scanlineStyle = nullptr) const
        {
            if (!shell.isLoaded())
            {
                return;
            }

            const Rect monitorRect = resolveMonitorRect(hostRegion);

            if (monitorRect.size.width <= 0 || monitorRect.size.height <= 0 || lines.empty())
            {
                return;
            }

            for (int row = 0; row < monitorRect.size.height; ++row)
            {
                const std::string& sourceLine =
                    lines[static_cast<std::size_t>((firstLineIndex + row) % static_cast<int>(lines.size()))];

                std::string renderedLine = "> " + sourceLine;

                if (static_cast<int>(renderedLine.size()) > monitorRect.size.width)
                {
                    renderedLine.resize(static_cast<std::size_t>(monitorRect.size.width));
                }

                buffer.writeString(
                    monitorRect.position.x,
                    monitorRect.position.y + row,
                    renderedLine,
                    textStyle);
            }

            if (scanlineStyle != nullptr)
            {
                for (int row = 0; row < monitorRect.size.height; row += 2)
                {
                    const int y = monitorRect.position.y + row;
                    const std::string scanLine(
                        static_cast<std::size_t>(monitorRect.size.width),
                        '-');

                    buffer.writeString(
                        monitorRect.position.x,
                        y,
                        scanLine,
                        *scanlineStyle);
                }
            }
        }
    };
}

///////////////////////////////////////////////////////////////////////////////////////////
// S H O W C A S E   C O N T R O L L E R

void TimedShowcaseScreen::onEnter()
{
    m_elapsedSeconds = 0.0;
}

void TimedShowcaseScreen::update(double deltaTime)
{
    m_elapsedSeconds += deltaTime;
}

double TimedShowcaseScreen::elapsedSeconds() const
{
    return m_elapsedSeconds;
}

///////////////////////////////////////////////////////////////////////////////////////////
// C O N T R O L   D E C K

void ControlDeckScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();
    if (isTooSmall(buffer))
    {
        drawTooSmallMessage(buffer, "ControlDeck", ControlAccent);
        return;
    }

    fillVerticalGradient(
        buffer,
        Rect{ Point{ 0, 0 }, Size{ buffer.getWidth(), buffer.getHeight() } },
        makeThemeColor(Color::Basic::BrightWhite, 255, 240, 247, 255),
        { Color::RgbValue{ 7, 14, 28 }, Color::RgbValue{ 10, 30, 55 } },
        Color::Basic::Black,
        17);

    PageComposer page(buffer);
    page.clearRegions();

    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Safe", "Screen", 2, 1, 2, 1);

    const Rect safe = page.resolveRegion("Safe");
    const auto [header, afterHeader] = page.splitTop(safe, 9);
    const auto [footer, body] = page.splitBottom(afterHeader, 4);
    const auto [leftPane, rightPane] = page.splitLeft(body, std::max(34, body.size.width / 2 - 3));

    page.createRegion("Header", header);
    page.createRegion("Footer", footer);
    page.createRegion("LeftPane", leftPane);
    page.createRegion("RightPane", rightPane);

    const auto [pipelinePane, afterPipeline] = page.splitTop(rightPane, 8);
    const auto [modesPane, authorPane] = page.splitTop(afterPipeline, 12);
    const Rect tickerPane = page.peekBottom(authorPane, 3);
    const Rect notesPane = page.remainderAbove(authorPane, 3);

    page.createRegion("PipelinePane", pipelinePane);
    page.createRegion("ModesPane", modesPane);
    page.createRegion("NotesPane", notesPane);
    page.createRegion("TickerPane", tickerPane);

    page.drawPanel("Header", ControlPanel, ControlFrame, ObjectFactory::doubleLineBorder());
    page.drawPanel("LeftPane", ControlPanel, ControlFrame);
    page.drawPanel("PipelinePane", ControlPanel, ControlFrame);
    page.drawPanel("ModesPane", ControlPanel, ControlFrame);
    page.drawPanel("NotesPane", ControlPanel, ControlFrame);
    page.drawPanel("TickerPane", ControlPanel, ControlFrame);
    page.drawPanel("Footer", ControlPanel, ControlFrame, ObjectFactory::roundedBorder());

    page.createInsetRegion("HeaderContent", "Header", 2, 1, 2, 1);
    page.createInsetRegion("LeftContent", "LeftPane", 2, 3, 2, 2);
    page.createInsetRegion("PipelineContent", "PipelinePane", 2, 3, 2, 1);
    page.createInsetRegion("ModesContent", "ModesPane", 2, 2, 2, 1);
    page.createInsetRegion("NotesContent", "NotesPane", 2, 3, 2, 1);
    page.createInsetRegion("TickerContent", "TickerPane", 2, 1, 2, 1);
    page.createInsetRegion("FooterContent", "Footer", 2, 1, 2, 1);

    const TextObject banner = makeBanner(
        "ANSI Shadow.flf",
        "CONTROL DECK",
        ControlInfo,
        AsciiBanner::ComposeMode::FullWidth);

    page.writeObjectInRegion(banner, "HeaderContent", Composition::Align::topCenter());
    page.writeTextInRegion("Layout Authoring / 24-bit Panels / Retained Objects", "HeaderContent", Composition::Align::bottomCenter());

    page.writePanelTitle("LeftPane", "LAYOUT + PANELS", ControlAccent);
    page.writePanelTitle("PipelinePane", "PIPELINES", ControlAccent);
    page.writePanelTitle("ModesPane", "OBJECT MODES", ControlAccent);
    page.writePanelTitle("NotesPane", "What API's new developers should learn", ControlAccent);

    page.writeWrappedTextInRegion(
        "TUI is not a layout engine - it's a composition engine.\n\nYou explicitly create and place everything. There are no hidden systems reflowing or shifting your UI.Nothing moves unless you tell it to.\n\nThis is what allows TUI applications to scale cleanly and remain predictable.\n\nThe Control Deck screen demonstrates how to organize a full application screen using simple, composable primitives.\n\nThe TUI Philosophy\n\nSimple primitives build complex screens.\n\nInstead of relying on complex frameworks with fragile behavior,\nTUI favors explicit composition and deterministic results.",
        "LeftContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Top });
    TextObject puzzleLine = ObjectFactory::makeHorizontalPatternLine(
        page.resolveRegion("LeftContent").size.width,
        ObjectFactory::puzzleLinePattern(),
        ControlAccent);

    page.writeObjectInRegion(puzzleLine, "LeftContent", Composition::Align::bottomLeft());

    const double ingestRatio = pulse(elapsedSeconds(), 0.85, 0.2);
    const double composeRatio = pulse(elapsedSeconds(), 1.20, 1.1);
    const double renderRatio = pulse(elapsedSeconds(), 0.55, 2.1);

    page.writeTextBlockInRegion(
        "BUILD OBJECTS        " + makeProgressBar(22, ingestRatio, '#') + "\n"
        "COMPOSE PAGES        " + makeProgressBar(22, composeRatio, '=') + "\n"
        "CREATE EXPERIENCES   " + makeProgressBar(22, renderRatio, '>'),
        "PipelineContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center });

    page.writeTextBlockInRegion(
        "All write modes answer one question: What parts of this object are allowed to affect the screen?\n\n"
        "SOLID OBJECT:Everything, including spaces, empty cells, and style\n"
        "VISIBLE OBJECT: Only visible glyphs and Style, no spaces or empty cells\n"
        "AUTHORED OBJECT: Only authored cells and Style (Glyphs + spaces, no empty cells)\n"
        "GLYPHS ONLY: Glyphs only (No spaces, Empties, or Style)\n"
        "STYLE MASK: Apply style where the object has shape\n"
        "STYLE BLOCK: Apply style over a rectangle",
        "ModesContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center });

    page.writeTextBlockInRegion(
        "• TextObject, ObjectFactory, ObjectComposer pipeline: How to build things\n"
        "• ScreenComposer, Draw Modes, Write Policy: How to place things\n"
        "• Layering and Style Effects: How to make things pretty\n"
        "• Asset Manager: How to load external assets like .xp files\n"
        "• Final mental model: Build objects. Control how they write. Draw them.",
        "NotesContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center });

    const int tickerWidth = std::max(1, page.resolveRegion("TickerContent").size.width);
    page.writeTextInRegion(
        makeMarquee(
            "control deck  •  layouts  •  banners  •  wrapped copy  •  framed telemetry  •  rgb accents",
            tickerWidth,
            static_cast<int>(elapsedSeconds() * 12.0)),
        "TickerContent",
        Composition::Align::centerLeft());

    page.writeTextInRegion("ControlDeckScreen  |  Page 1 of a Multi-Scene Showcase", "FooterContent", Composition::Align::centerLeft());
    page.writeTextInRegion("24-bit gradient background + dedicated scene class", "FooterContent", Composition::Align::centerRight());
}

///////////////////////////////////////////////////////////////////////////////////////////
// R E T R O   T E R M I N A L

RetroTerminalScreen::RetroTerminalScreen(Assets::AssetLibrary& assetLibrary)
    : m_assetLibrary(assetLibrary)
{
}

void RetroTerminalScreen::onEnter()
{
    TimedShowcaseScreen::onEnter();

    if (m_retroTerminalLoadAttempted)
    {
        return;
    }

    m_retroTerminalLoadAttempted = true;

    const auto loadResult = m_assetLibrary.loadTextAsset("xp.retro_terminal_1");
    if (!loadResult.success || !loadResult.asset.object)
    {
        m_retroTerminalLoadError = loadResult.errorMessage;
        return;
    }

    m_retroTerminalObject = *loadResult.asset.object;
}

void RetroTerminalScreen::onExit()
{
    m_retroTerminalObject.clear();
}

void RetroTerminalScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();
    if (isTooSmall(buffer))
    {
        drawTooSmallMessage(buffer, "RetroTerminal", TerminalFrame);
        return;
    }
    
    fillVerticalGradient(
        buffer,
        Rect{ Point{ 0, 0 }, Size{ buffer.getWidth(), buffer.getHeight() } },
        makeThemeColor(Color::Basic::BrightGreen, 46, 126, 255, 168),
        { Color::RgbValue{ 0, 7, 0 }, Color::RgbValue{ 5, 22, 8 } },
        Color::Basic::Black,
        16);

    PageComposer page(buffer);
    page.clearRegions();
    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Safe", "Screen", 2, 1, 2, 1);

    const Rect safe = page.resolveRegion("Safe");
    const auto [header, afterHeader] = page.splitTop(safe, 7);
    const auto [footer, body] = page.splitBottom(afterHeader, 5);
    const auto [consolePane, telemetryPane] =
        page.splitLeft(body, std::max(44, (body.size.width * 3) / 5));

    page.createRegion("Header", header);
    page.createRegion("Footer", footer);
    page.createRegion("ConsolePane", consolePane);
    page.createRegion("TelemetryPane", telemetryPane);

    page.drawPanel("Header", TerminalHeaderSurface, TerminalHeaderSurface, ObjectFactory::doubleLineBorder());
    page.drawPanel("Footer", TerminalPanel, TerminalFrame, ObjectFactory::doubleLineBorder());
    page.drawPanel("ConsolePane", TerminalPanel, TerminalFrame);
    page.drawPanel("TelemetryPane", TerminalPanel, TerminalFrame);

    page.createInsetRegion("HeaderContent", "Header", 2, 0, 2, 0);
    page.createInsetRegion("FooterContent", "Footer", 2, 1, 2, 1);
    page.createInsetRegion("ConsoleContent", "ConsolePane", 2, 2, 2, 2);
    page.createInsetRegion("TelemetryContent", "TelemetryPane", 2, 3, 2, 2);

    page.writePanelTitle("ConsolePane", ".xp Styled Components", TerminalFrame);
    page.writePanelTitle("TelemetryPane", "SIGNAL ANALYSIS", TerminalFrame);

    const Rect consoleContentRegion = page.resolveRegion("ConsoleContent");
    const TextObject heartColumn = ObjectFactory::makeVerticalPatternLine(consoleContentRegion.size.height, ObjectFactory::heartChainVerticalPattern());
    page.writeObjectInRegion(heartColumn, "ConsoleContent", Composition::Align::topRight());

    const TextObject banner = makeBanner("Computer.flf", "retro terminal", TerminalHeaderSurface);
    page.writeObjectInRegion(banner, "HeaderContent", Composition::Align::topCenter());

    const std::vector<std::string> logLines =
    {
        "[boot] screen host bound",
        "[boot] renderer negotiated",
        "[scan] banner fonts available",
        "[scan] unicode clusters ok",
        "[pass] composition surface warm",
        "[pass] terminal palette resolved",
        "[demo] scrolling region healthy",
        "[demo] faux telemetry stable",
        "[demo] phosphor theme alive",
        "[ok] scene extracted cleanly"
    };

    const int baseIndex = static_cast<int>(elapsedSeconds() * 4.0);
    Rect consoleContent = page.resolveRegion("ConsoleContent");

    consoleContent.position.y++;

    RetroTerminalComponent retroTerminal
    {
        m_retroTerminalObject,
        Rect
        {
            Point{ 2, 1 },
            Size{ 35, 10 }
        }
    };

    if (retroTerminal.isLoaded())
    {
        retroTerminal.drawShell(buffer, consoleContent, solidObject());

        retroTerminal.drawLogLines(
            buffer,
            consoleContent,
            logLines,
            baseIndex,
            TerminalSurface,
            nullptr);
    }
    else
    {
        std::string logBlock;
        for (int i = 0; i < 10; ++i)
        {
            logBlock += "> ";
            logBlock += logLines[static_cast<std::size_t>((baseIndex + i) % static_cast<int>(logLines.size()))];
            if (i + 1 < 10)
            {
                logBlock += '\n';
            }
        }

        page.writeTextBlockInRegion(
            logBlock,
            "ConsoleContent",
            Composition::Align::topLeft(),
            authoredObject());
    }

    page.writeWrappedTextInRegion(
        "TUI can emulate a terminal-style text-driven experience using the same primitives as UI. Create a clean split between console output and side telemetry. Use FIGlet, TOIlet, and Psuedo fonts to create eye catching titles. All with character level control. Dynamic rendering, not just static layouts.",
        "TelemetryContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Top });

    page.writeObjectInRegion(waveObject(), "TelemetryContent", Composition::Align::center());
    page.writeObjectInRegion(verticalMeterObject(), "TelemetryContent", Composition::Align::centerRight());
    page.writeTextInRegion(
        makeProgressBar(24, pulse(elapsedSeconds(), 0.65), '*', ' '),
        "TelemetryContent",
        Composition::Align::bottomLeft());

    const int footerWidth = std::max(1, page.resolveRegion("FooterContent").size.width);
    page.writeTextInRegion(
        makeMarquee(
            "monochrome terminal mode  •  retained text objects  •  status scan  •  telemetry overlay  •  new dedicated screen",
            footerWidth,
            static_cast<int>(elapsedSeconds() * 18.0)),
        "FooterContent",
        Composition::Align::centerLeft());

    page.writePanelTitle("Footer", "SHOWCASE / RETRO TERMINAL", TerminalInverse);
}

///////////////////////////////////////////////////////////////////////////////////////////
// N E O N   D I A L O G

void NeonDialogScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();
    if (isTooSmall(buffer))
    {
        drawTooSmallMessage(buffer, "NeonDialog", NeonGold);
        return;
    }

    fillVerticalGradient(
        buffer,
        Rect{ Point{ 0, 0 }, Size{ buffer.getWidth(), buffer.getHeight() } },
        makeThemeColor(Color::Basic::BrightWhite, 255, 255, 244, 255),
        { Color::RgbValue{ 12, 5, 23 }, Color::RgbValue{ 34, 7, 56 } },
        Color::Basic::Black,
        17);

    PageComposer page(buffer);
    page.clearRegions();

    const Rect screen = page.getFullScreenRegion();
    page.createRegion("FullPage", screen);
  
    const Style fullOuterFrameStyle =
        style::Fg(makeThemeColor(Color::Basic::BrightMagenta, 213, 255, 86, 214))
        + style::Bold;

    TextObject fullOuterFrame =
        ObjectFactory::makePatternFrame(
            screen.size.width,
            screen.size.height,
            ObjectFactory::heartFramePattern(),
            fullOuterFrameStyle);

    page.writeObjectInRegion(
        fullOuterFrame,
        "FullPage",
        Composition::Align::center(),
        visibleObject());

    const Rect dialog = Rect
    {
        Point{ std::max(2, (screen.size.width - 96) / 2), std::max(2, (screen.size.height - 26) / 2) },
        Size { std::min(96, screen.size.width), std::min(26, screen.size.height) }
    };

    page.createRegion("Dialog", dialog);
    page.drawPanel("Dialog", NeonDialogFill, NeonFrame, ObjectFactory::roundedBorder());
    page.createInsetRegion("DialogSafe", "Dialog", 1, 1, 1, 1);

    const double shimmer = pulse(elapsedSeconds(), 1.35);

    const Style bannerStyle = makeStyle(
        makeThemeColor(Color::Basic::BrightCyan, 123, 155, 250, 255),
        makeThemeColor(Color::Basic::Magenta, 91,
            static_cast<std::uint8_t>(55 + std::round(shimmer * 30.0)),
            22,
            static_cast<std::uint8_t>(84 + std::round(shimmer * 24.0))),
        true);

    const Rect safe = page.resolveRegion("DialogSafe");
    const auto [header, afterHeader] = page.splitTop(safe, 6);
    const auto [footer, body] = page.splitBottom(afterHeader, 4);
    const auto [previewPane, copyPane] =
        page.splitLeft(body, std::max(32, body.size.width / 2));

    page.createRegion("Header", header);
    page.createRegion("Footer", footer);
    page.createRegion("PreviewPane", previewPane);
    page.createRegion("CopyPane", copyPane);

    page.drawPanel("PreviewPane", NeonDialogFill, NeonFrame);
    page.drawPanel("CopyPane", NeonDialogFill, NeonFrame);

    page.createInsetRegion("HeaderContent", "Header", 2, 0, 2, 0);
    page.createInsetRegion("PreviewContent", "PreviewPane", 2, 3, 2, 2);
    page.createInsetRegion("CopyContent", "CopyPane", 2, 3, 2, 2);
    page.createInsetRegion("FooterContent", "Footer", 2, 1, 2, 1);

    page.writePanelTitle("PreviewPane", "PREVIEW", NeonGold);
    page.writePanelTitle("CopyPane", "FEATURES", NeonGold);
    page.writePanelTitle("Dialog", "SHOWCASE / NEON DIALOG", NeonGold, 3, -1);

    const TextObject banner = makeBanner("Slant.flf", "Neon UI", bannerStyle);
    page.writeObjectInRegion(banner, "HeaderContent", Composition::Align::topCenter());

    page.writeWrappedTextInRegion(
        "The dialog scene works well for launchers, settings panes, and modal workflows. Extracting it into its own class makes it easier to evolve separately from the other showcase looks.",
        "PreviewContent",
        Alignment{ Composition::HorizontalAlign::Center, Composition::VerticalAlign::Center });
    
    page.writeTextBlockInRegion(
        "• dedicated modal-style screen\n"
        "• stronger 24-bit magenta / cyan identity\n"
        "• clean header-body-footer split\n"
        "• separate preview and copy panels\n"
        "• easier future expansion into real menus",
        "CopyContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center });

    page.writeTextInRegion(makeProgressBar(24, pulse(elapsedSeconds(), 0.95, 1.0), '@', '.'), "FooterContent", Composition::Align::centerLeft());
    page.writeTextInRegion("[ ENTER ] Launch   [ TAB ] Theme   [ ESC ] Back", "FooterContent", Composition::Align::centerRight());
}

///////////////////////////////////////////////////////////////////////////////////////////
// O P S   W A L L

void OpsWallScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();
    if (isTooSmall(buffer))
    {
        drawTooSmallMessage(buffer, "OpsWall", OpsAmber);
        return;
    }

    fillVerticalGradient(
        buffer,
        Rect{ Point{ 0, 0 }, Size{ buffer.getWidth(), buffer.getHeight() } },
        makeThemeColor(Color::Basic::BrightWhite, 255, 240, 240, 240),
        { Color::RgbValue{ 8, 8, 10 }, Color::RgbValue{ 28, 18, 18 } },
        Color::Basic::Black,
        16);

    PageComposer page(buffer);
    page.clearRegions();

    page.createFullScreenRegion("Screen");
    page.createInsetRegion("Canvas", "Screen", 1, 1, 1, 1);

    const Rect canvas = page.resolveRegion("Canvas");
    const auto [header, afterHeader] = page.splitTop(canvas, 8);
    const auto [footer, body] = page.splitBottom(afterHeader, 4);
    const auto [leftColumn, rightColumn] = page.splitLeft(body, std::max(40, (body.size.width * 2) / 3));
    const auto [statusPanel, heatPanel] = page.splitTop(leftColumn, std::max(9, leftColumn.size.height / 2));
    const auto [metricsPanel, alertPanel] = page.splitTop(rightColumn, rightColumn.size.height - 10);

    page.createRegion("Header", header);
    page.createRegion("Footer", footer);
    page.createRegion("StatusPanel", statusPanel);
    page.createRegion("HeatPanel", heatPanel);
    page.createRegion("MetricsPanel", metricsPanel);
    page.createRegion("AlertPanel", alertPanel);

    page.drawPanel("Header", OpsPanel, OpsFrame, ObjectFactory::doubleLineBorder());
    page.drawPanel("StatusPanel", OpsPanel, OpsFrame);
    page.drawPanel("HeatPanel", OpsPanel, OpsFrame);
    page.drawPanel("MetricsPanel", OpsPanel, OpsFrame);
    page.drawPanel("AlertPanel", OpsPanel, OpsFrame);
    page.drawPanel("Footer", OpsPanel, OpsFrame, ObjectFactory::doubleLineBorder());

    page.createInsetRegion("HeaderContent", "Header", 2, 1, 2, 1);
    page.createInsetRegion("StatusContent", "StatusPanel", 2, 3, 2, 1);
    page.createInsetRegion("HeatContent", "HeatPanel", 2, 3, 2, 1);
    page.createInsetRegion("MetricsContent", "MetricsPanel", 2, 3, 2, 1);
    page.createInsetRegion("AlertContent", "AlertPanel", 2, 3, 2, 1);
    page.createInsetRegion("FooterContent", "Footer", 2, 1, 2, 1);

    page.writePanelTitle("StatusPanel", "NODE STATUS GRID", OpsAmber);
    page.writePanelTitle("HeatPanel", "LOAD HEAT MAP", OpsAmber);
    page.writePanelTitle("MetricsPanel", "SYSTEM METRICS", OpsAmber);
    page.writePanelTitle("AlertPanel", "ALERT CHANNEL", OpsAmber);

    const TextObject banner = makeBanner(
        "Small Shadow.flf",
        "TUI Ops Wall",
        OpsAmber,
        AsciiBanner::ComposeMode::FullWidth);

    page.writeObjectInRegion(banner, "HeaderContent", Composition::Align::centerLeft());

    const int modeIndex = static_cast<int>(elapsedSeconds() / 10.0) % 3;
    const char* modeName = (modeIndex == 0) ? "CALM" : (modeIndex == 1) ? "BUSY" : "CHAOTIC";
    const double modeLoad = (modeIndex == 0) ? 0.28 : (modeIndex == 1) ? 0.62 : 0.86;

    const double cpu = std::clamp(modeLoad + (pulse(elapsedSeconds(), 1.7, 0.0) - 0.5) * 0.34, 0.0, 1.0);
    const double memory = std::clamp(modeLoad + (pulse(elapsedSeconds(), 1.1, 1.2) - 0.5) * 0.26, 0.0, 1.0);
    const double network = std::clamp(modeLoad + (pulse(elapsedSeconds(), 2.2, 2.0) - 0.5) * 0.42, 0.0, 1.0);
    const double queue = std::clamp(modeLoad + (pulse(elapsedSeconds(), 0.9, 3.4) - 0.5) * 0.50, 0.0, 1.0);

    const bool cpuAlert = cpu >= 0.82;
    const bool memoryAlert = memory >= 0.78;
    const bool networkAlert = network >= 0.88;
    const bool queueAlert = queue >= 0.80;
    const bool anyAlert = cpuAlert || memoryAlert || networkAlert || queueAlert;

    const Style alertStyle = anyAlert ? OpsFrame + style::SlowBlink : OpsAmber;
    const Style hotStyle = OpsFrame + style::Bold;
    const Style warmStyle = OpsAmber;
    const Style coolStyle = OpsCyan;

    buffer.writeString(header.position.x + header.size.width - 24, header.position.y + 2, std::string("SIM MODE: ") + modeName, alertStyle);

    const Rect statusContent = page.resolveRegion("StatusContent");
    const TextObject dividerBox = ObjectFactory::makeDividerBox(
        statusContent.size.width,
        statusContent.size.height,
        3,
        2,
        U' ',
        OpsCyan,
        ObjectFactory::singleLineGlyphs());

    page.writeObjectInRegion(dividerBox, "StatusContent", Composition::Align::topLeft(), visibleObject());

    const std::array<std::string, 12> nodeLabels =
    {
        "API", "DB", "CACHE", "BUS",
        "AUTH", "JOBS", "CDN", "MQ",
        "GPU", "LOG", "AI", "EDGE"
    };

    const int nodeCellWidth = std::max(1, statusContent.size.width / 4);
    const int nodeCellHeight = std::max(1, statusContent.size.height / 3);

    for (int i = 0; i < 12; ++i)
    {
        const int col = i % 4;
        const int row = i / 4;
        const double load = std::clamp(
            modeLoad + (pulse(elapsedSeconds(), 0.75 + (i * 0.11), i * 0.73) - 0.5) * 0.55,
            0.0,
            1.0);

        const Style nodeStyle = (load > 0.82) ? hotStyle : (load > 0.58) ? warmStyle : coolStyle;
        const int x = statusContent.position.x + (col * nodeCellWidth) + 1;
        const int y = statusContent.position.y + (row * nodeCellHeight) + std::max(0, nodeCellHeight / 2);

        buffer.writeString(x, y, nodeLabels[static_cast<std::size_t>(i)], nodeStyle);
    }

    const Rect heatContent = page.resolveRegion("HeatContent");
    const int heatColumns = std::max(4, std::min(16, heatContent.size.width / 2));
    const int heatRows = std::max(3, std::min(7, heatContent.size.height));

    const TextObject heatGrid = ObjectFactory::makeGrid(
        heatColumns,
        heatRows,
        1,
        1,
        OpsCyan,
        ObjectFactory::singleLineGlyphs());

    page.writeObjectInRegion(heatGrid, "HeatContent", Composition::Align::topLeft(), visibleObject());

    for (int y = 0; y < heatRows; ++y)
    {
        for (int x = 0; x < heatColumns; ++x)
        {
            const double intensity = std::clamp(
                modeLoad + (pulse(elapsedSeconds(), 1.0 + (x * 0.13), y * 0.91) - 0.5) * 0.70,
                0.0,
                1.0);

            const char glyph = (intensity > 0.82) ? '#' : (intensity > 0.58) ? '+' : '.';
            const Style cellStyle = (intensity > 0.82) ? hotStyle : (intensity > 0.58) ? warmStyle : coolStyle;

            const int drawX = heatContent.position.x + 1 + (x * 2);
            const int drawY = heatContent.position.y + 1 + (y * 2);

            if (buffer.inBounds(drawX, drawY))
            {
                buffer.writeChar(drawX, drawY, glyph, cellStyle);
            }
        }
    }

    const std::string metricsBlock =
        std::string("CPU     ") + makeProgressBar(18, cpu, '#') + "\n"
        + "MEMORY  " + makeProgressBar(18, memory, '=') + "\n"
        + "NET I/O " + makeProgressBar(18, network, '>') + "\n"
        + "QUEUE   " + makeProgressBar(18, queue, '*') + "\n\n"
        + "Simulation mode cycles every 10 seconds.\n"
        + "Calm -> Busy -> Chaotic";

    page.writeTextBlockInRegion(
        metricsBlock,
        "MetricsContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Top });

    std::string alertText;
    if (!anyAlert)
    {
        alertText = "All systems nominal\nNo active threshold events";
    }
    else
    {
        alertText = "ACTIVE ALERTS\n";
        if (cpuAlert) { alertText += "- CPU saturation rising\n"; }
        if (memoryAlert) { alertText += "- Memory pressure high\n"; }
        if (networkAlert) { alertText += "- Network burst detected\n"; }
        if (queueAlert) { alertText += "- Work queue backing up\n"; }
    }

    page.writeTextBlockInRegion(
        alertText,
        "AlertContent",
        Alignment{ Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center },
        authoredObject());

    const int footerWidth = std::max(1, page.resolveRegion("FooterContent").size.width);
    page.writeTextInRegion(
        makeMarquee(
            "ops wall  |  divider boxes  |  grid heat maps  |  live metrics  |  alert thresholds  |  procedural overlays",
            footerWidth,
            static_cast<int>(elapsedSeconds() * (anyAlert ? 20.0 : 10.0))),
        "FooterContent",
        Composition::Align::centerLeft());

    for (int x = 3; x < buffer.getWidth() - 3; x += 9)
    {
        const int y = static_cast<int>(2 + std::fmod(elapsedSeconds() * (anyAlert ? 9.0 : 4.0) + static_cast<double>(x), 6.0));
        if (buffer.inBounds(x, y))
        {
            buffer.writeChar(x, y, anyAlert ? '!' : '*', anyAlert ? hotStyle : OpsCyan);
        }
    }
}
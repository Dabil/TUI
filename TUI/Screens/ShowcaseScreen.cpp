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

    Alignment makeAlignment(
        Composition::HorizontalAlign horizontal,
        Composition::VerticalAlign vertical)
    {
        return { horizontal, vertical };
    }

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

    void writeText(
        PageComposer& page,
        std::string_view text,
        std::string_view regionName,
        const Alignment& alignment,
        const Composition::WritePolicy& policy = visibleObject())
    {
        page.writeAlignedText(
            text,
            PageComposer::PlacementSpec::inNamedRegion(regionName, alignment, true),
            policy);
    }

    void writeTextBlock(
        PageComposer& page,
        std::string_view text,
        std::string_view regionName,
        const Alignment& alignment,
        const Composition::WritePolicy& policy = visibleObject())
    {
        page.writeAlignedTextBlock(
            text,
            PageComposer::PlacementSpec::inNamedRegion(regionName, alignment, true),
            policy);
    }

    void writeWrapped(
        PageComposer& page,
        std::string_view text,
        std::string_view regionName,
        const Alignment& alignment,
        const Composition::WritePolicy& policy = visibleObject())
    {
        page.writeWrappedText(
            text,
            PageComposer::PlacementSpec::inNamedRegion(regionName, alignment, true),
            policy);
    }

    void writeObject(
        PageComposer& page,
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const Composition::WritePolicy& policy = visibleObject())
    {
        page.writeObject(
            object,
            PageComposer::PlacementSpec::inNamedRegion(regionName, alignment, true),
            policy);
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

    const TextObject& cubeObject()
    {
        static const TextObject object = TextObject::fromUtf8(
            "   _________ \n"
            "  / _______/|\n"
            " / /______/ |\n"
            "/_/______/  |\n"
            "|  TUI   |  |\n"
            "| ENGINE | / \n"
            "|________|/  ");
        return object;
    }

    const TextObject& chipObject()
    {
        static const TextObject object = TextObject::fromUtf8(
            "   .--------------.\n"
            "  /     TUI      /|\n"
            " / BuildS with: / |\n"
            "+--------------+  |\n"
            "| TEXT OBJECTS |  |\n"
            "| COLOR STYLES |  .\n"
            "|  .XP FILES   | / \n"
            "|   WIDGETS    |/  \n"
            "+--------------+  ");
        return object;
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

    const TextObject& satelliteObject()
    {
        static const TextObject object = TextObject::fromUtf8(
            "        .-.        \n"
            "    --- ( ) ---    \n"
            "        `-'        \n"
            "     .--| |--.     \n"
            "    /___| |___\\   \n"
            "        / \\       \n"
            "       /___\\      ");
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

    const Rect screen = page.getFullScreenRegion();
    const Rect safe = insetRect(screen, 2, 1, 2, 1);
    const auto [header, afterHeader] = page.splitTop(safe, 9);
    const auto [footer, body] = page.splitBottom(afterHeader, 4);
    const auto [leftPane, rightPane] = page.splitLeft(body, std::max(34, body.size.width / 2 - 3));

    page.createRegion("Header", header);
    page.createRegion("Footer", footer);
    page.createRegion("LeftPane", leftPane);
    page.createRegion("RightPane", rightPane);

    const auto [pipelinePane, afterPipeline] = page.splitTop(insetRect(rightPane, 0, 0, 0, 0), 8);
    const auto [modesPane, authorPane] = page.splitTop(afterPipeline, 12);
    const Rect tickerPane = page.peekBottom(authorPane, 3);
    const Rect notesPane = page.remainderAbove(authorPane, 3);

    paintPanel(buffer, header, ControlPanel, ControlFrame, ObjectFactory::doubleLineBorder());
    paintPanel(buffer, leftPane, ControlPanel, ControlFrame);
    paintPanel(buffer, pipelinePane, ControlPanel, ControlFrame);
    paintPanel(buffer, modesPane, ControlPanel, ControlFrame);
    paintPanel(buffer, notesPane, ControlPanel, ControlFrame);
    paintPanel(buffer, tickerPane, ControlPanel, ControlFrame);
    paintPanel(buffer, footer, ControlPanel, ControlFrame, ObjectFactory::roundedBorder());

    page.createRegion("HeaderContent", insetRect(header, 2, 1, 2, 1));
    page.createRegion("LeftContent", insetRect(leftPane, 2, 3, 2, 2));
    page.createRegion("PipelineContent", insetRect(pipelinePane, 2, 3, 2, 1));
    page.createRegion("ModesContent", insetRect(modesPane, 2, 2, 2, 1));
    page.createRegion("NotesContent", insetRect(notesPane, 2, 3, 2, 1));
    page.createRegion("TickerContent", insetRect(tickerPane, 2, 1, 2, 1));
    page.createRegion("FooterContent", insetRect(footer, 2, 1, 2, 1));

    const TextObject banner = makeBanner(
        "ANSI Shadow.flf",
        "CONTROL DECK",
        ControlInfo,
        AsciiBanner::ComposeMode::FullWidth);

    writeObject(page, banner, "HeaderContent", Composition::Align::topCenter());
    writeText(page, "Layout Authoring / 24-bit Panels / Retained Objects", "HeaderContent", Composition::Align::bottomCenter());

    buffer.writeString(leftPane.position.x + 2, leftPane.position.y + 1, "LAYOUT + PANELS", ControlAccent);
    buffer.writeString(pipelinePane.position.x + 2, pipelinePane.position.y + 1, "PIPELINES", ControlAccent);
    buffer.writeString(modesPane.position.x + 2, modesPane.position.y + 1, "OBJECT MODES", ControlAccent);
    buffer.writeString(notesPane.position.x + 2, notesPane.position.y + 1, "WHY THIS PORT FITS THE NEW REPO", ControlAccent);

    writeWrapped(
        page,
        "The old combined showcase scene becomes a dedicated ControlDeck screen. This version leans harder into the new repository's richer color pipeline, clearer panel hierarchy, and placement-based composition instead of keeping all four looks inside one cycling screen.",
        "LeftContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Top));

    writeObject(page, cubeObject(), "LeftContent", Composition::Align::bottomCenter());

    const double ingestRatio = pulse(elapsedSeconds(), 0.85, 0.2);
    const double composeRatio = pulse(elapsedSeconds(), 1.20, 1.1);
    const double renderRatio = pulse(elapsedSeconds(), 0.55, 2.1);

    writeTextBlock(
        page,
        "ASSET INGEST   " + makeProgressBar(22, ingestRatio, '#') + "\n"
        "PAGE COMPOSE   " + makeProgressBar(22, composeRatio, '=') + "\n"
        "PRESENT PASS   " + makeProgressBar(22, renderRatio, '>'),
        "PipelineContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center));

    writeObject(page, chipObject(), "ModesContent", Composition::Align::centerLeft());
    writeTextBlock(
        page,
        "SOLID OBJECT\n"
        "VISIBLE OBJECT\n"
        "GLYPHS ONLY\n"
        "STYLE MASK",
        "ModesContent",
        makeAlignment(Composition::HorizontalAlign::Right, Composition::VerticalAlign::Center));

    writeTextBlock(
        page,
        "• split into its own screen class\n"
        "• richer RGB-themed panels\n"
        "• clearer content regions\n"
        "• cleaner integration path into ScreenManager\n"
        "• no dependency on a 4-scene timer hub",
        "NotesContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center));

    const int tickerWidth = std::max(1, page.resolveRegion("TickerContent").size.width);
    writeText(
        page,
        makeMarquee(
            "control deck  •  layouts  •  banners  •  wrapped copy  •  framed telemetry  •  rgb accents",
            tickerWidth,
            static_cast<int>(elapsedSeconds() * 12.0)),
        "TickerContent",
        Composition::Align::centerLeft());

    writeText(page, "ControlDeckScreen  |  extracted from the original multi-scene ShowcaseScreen", "FooterContent", Composition::Align::centerLeft());
    writeText(page, "24-bit gradient background + dedicated scene class", "FooterContent", Composition::Align::centerRight());
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

    const Rect screen = page.getFullScreenRegion();
    const Rect safe = insetRect(screen, 2, 1, 2, 1);
    const auto [header, afterHeader] = page.splitTop(safe, 7);
    const auto [footer, body] = page.splitBottom(afterHeader, 5);
    const auto [consolePane, telemetryPane] = page.splitLeft(body, std::max(44, (body.size.width * 3) / 5));

    TextObject headerObject = ObjectFactory::makeFrame(header.size.width, header.size.height, TerminalHeaderSurface, ObjectFactory::doubleLineBorder());

    headerObject.draw(buffer, header.position.x, header.position.y);

    paintPanel(buffer, footer, TerminalPanel, TerminalFrame, ObjectFactory::doubleLineBorder());
    paintPanel(buffer, consolePane, TerminalPanel, TerminalFrame, ObjectFactory::singleLineBorder());
    paintPanel(buffer, telemetryPane, TerminalPanel, TerminalFrame, ObjectFactory::singleLineBorder());

    page.createRegion("HeaderContent", insetRect(header, 2, 0, 2, 0));
    page.createRegion("FooterContent", insetRect(footer, 2, 1, 2, 1));
    page.createRegion("ConsoleContent", insetRect(consolePane, 2, 3, 2, 2));
    page.createRegion("TelemetryContent", insetRect(telemetryPane, 2, 3, 2, 2));

    buffer.writeString(consolePane.position.x + 2, consolePane.position.y + 1, ".xp Styled Components", TerminalFrame);
    buffer.writeString(telemetryPane.position.x + 2, telemetryPane.position.y + 1, "SIGNAL ANALYSIS", TerminalFrame);

    const TextObject banner = makeBanner("Computer.flf", "retro terminal", TerminalHeaderSurface);
    writeObject(page, banner, "HeaderContent", Composition::Align::topCenter());

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
    const Rect consoleContent = page.resolveRegion("ConsoleContent");

    RetroTerminalComponent retroTerminal
    {
        m_retroTerminalObject,
        Rect{
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

        page.writeAlignedTextBlock(
            logBlock,
            PageComposer::PlacementSpec::inNamedRegion("ConsoleContent", Composition::Align::topLeft(), true),
            visibleObject());
    }

    writeWrapped(
        page,
        "This extracted screen keeps the old monochrome-terminal mood but upgrades it with richer greens, softer background falloff, and a cleaner split between console output and side telemetry.",
        "TelemetryContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Top));

    writeObject(page, waveObject(), "TelemetryContent", Composition::Align::center());
    writeObject(page, verticalMeterObject(), "TelemetryContent", Composition::Align::centerRight());
    writeText(
        page,
        makeProgressBar(24, pulse(elapsedSeconds(), 0.65), '*', ' '),
        "TelemetryContent",
        Composition::Align::bottomLeft());

    const int footerWidth = std::max(1, page.resolveRegion("FooterContent").size.width);
    writeText(
        page,
        makeMarquee(
            "monochrome terminal mode  •  retained text objects  •  status scan  •  telemetry overlay  •  new dedicated screen",
            footerWidth,
            static_cast<int>(elapsedSeconds() * 18.0)),
        "FooterContent",
        Composition::Align::centerLeft());

    buffer.writeString(footer.position.x + 2, footer.position.y + 1, "SHOWCASE / RETRO TERMINAL", TerminalInverse);
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
    const Rect dialog = Rect
    {
        Point{ std::max(2, (screen.size.width - 96) / 2), std::max(2, (screen.size.height - 26) / 2) },
        Size { std::min(96, screen.size.width), std::min(26, screen.size.height) }
    };

    paintPanel(buffer, dialog, NeonDialogFill, NeonFrame, ObjectFactory::roundedBorder());

    // neon size
    const Rect safe = insetRect(dialog, 1, 1, 1, 1);

    const auto [header, afterHeader] = page.splitTop(safe, 6);
    const auto [footer, body] = page.splitBottom(afterHeader, 4);
    const auto [previewPane, copyPane] = page.splitLeft(body, std::max(32, body.size.width / 2));

    paintPanel(buffer, previewPane, NeonDialogFill, NeonFrame, ObjectFactory::singleLineBorder());
    paintPanel(buffer, copyPane, NeonDialogFill, NeonFrame, ObjectFactory::singleLineBorder());

    page.createRegion("HeaderContent", insetRect(header, 2, 1, 2, 1));
    page.createRegion("PreviewContent", insetRect(previewPane, 2, 3, 2, 2));
    page.createRegion("CopyContent", insetRect(copyPane, 2, 3, 2, 2));
    page.createRegion("FooterContent", insetRect(footer, 2, 1, 2, 1));

    buffer.writeString(previewPane.position.x + 2, previewPane.position.y + 1, "PREVIEW", NeonGold);
    buffer.writeString(copyPane.position.x + 2, copyPane.position.y + 1, "FEATURES", NeonGold);

    const TextObject banner = makeBanner("Slant.flf", "Neon UI", NeonAccent);
    writeObject(page, banner, "HeaderContent", Composition::Align::center());

    const double shimmer = pulse(elapsedSeconds(), 1.35);
    const Style orbStyle = makeStyle(
        makeThemeColor(Color::Basic::BrightCyan, 123, 155, 250, 255),
        makeThemeColor(
            Color::Basic::Magenta,
            91,
            static_cast<std::uint8_t>(55 + std::round(shimmer * 30.0)),
            22,
            static_cast<std::uint8_t>(84 + std::round(shimmer * 24.0))),
        true);

    writeObject(page, satelliteObject(), "PreviewContent", Composition::Align::bottomCenter());
    writeWrapped(
        page,
        "The dialog scene works well for launchers, settings panes, and modal workflows. Extracting it into its own class makes it easier to evolve separately from the other showcase looks.",
        "PreviewContent",
        makeAlignment(Composition::HorizontalAlign::Center, Composition::VerticalAlign::Center));
    
    const TextObject orb = TextObject::fromUtf8(
        "    .-''''-.    \n"
        "  .'  .--.  '.  \n"
        " /   ( () )   \\\n"
        "|   .-====-.   |\n"
        " \\  '.__.'  /  \n"
        "  '.  '--' .'   \n"
        "    '-..-'      ",
        orbStyle);

    writeObject(page, orb, "CopyContent", Composition::Align::topCenter(), solidObject());

    writeTextBlock(
        page,
        "• dedicated modal-style screen\n"
        "• stronger 24-bit magenta / cyan identity\n"
        "• clean header-body-footer split\n"
        "• separate preview and copy panels\n"
        "• easier future expansion into real menus",
        "CopyContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center));

    writeText(page, makeProgressBar(24, pulse(elapsedSeconds(), 0.95, 1.0), '@', '.'), "FooterContent", Composition::Align::centerLeft());
    writeText(page, "[ ENTER ] Launch   [ TAB ] Theme   [ ESC ] Back", "FooterContent", Composition::Align::centerRight());

    buffer.writeString(dialog.position.x + 3, dialog.position.y - 1, "SHOWCASE / NEON DIALOG", NeonGold);
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
        { Color::RgbValue{ 8, 8, 10 }, Color::RgbValue{ 24, 18, 18 } },
        Color::Basic::Black,
        16);

    PageComposer page(buffer);
    page.clearRegions();

    const Rect screen = page.getFullScreenRegion();
    const Rect canvas = insetRect(screen, 1, 1, 1, 1);
    const auto [header, afterHeader] = page.splitTop(canvas, 8);
    const auto [footer, body] = page.splitBottom(afterHeader, 5);
    const auto [topBand, bottomBand] = page.splitTop(body, std::max(10, body.size.height / 2));
    const auto [topLeft, topRight] = page.splitLeft(topBand, std::max(36, topBand.size.width / 2));
    const auto [bottomLeft, bottomRight] = page.splitLeft(bottomBand, std::max(36, bottomBand.size.width / 2));

    paintPanel(buffer, header, OpsPanel, OpsFrame, ObjectFactory::doubleLineBorder());
    paintPanel(buffer, footer, OpsPanel, OpsFrame, ObjectFactory::doubleLineBorder());
    paintPanel(buffer, topLeft, OpsPanel, OpsFrame);
    paintPanel(buffer, topRight, OpsPanel, OpsFrame);
    paintPanel(buffer, bottomLeft, OpsPanel, OpsFrame);
    paintPanel(buffer, bottomRight, OpsPanel, OpsFrame);

    page.createRegion("HeaderContent", insetRect(header, 2, 1, 2, 1));
    page.createRegion("FooterContent", insetRect(footer, 2, 1, 2, 1));
    page.createRegion("TopLeftContent", insetRect(topLeft, 2, 3, 2, 2));
    page.createRegion("TopRightContent", insetRect(topRight, 2, 3, 2, 2));
    page.createRegion("BottomLeftContent", insetRect(bottomLeft, 2, 3, 2, 2));
    page.createRegion("BottomRightContent", insetRect(bottomRight, 2, 3, 2, 2));

    buffer.writeString(topLeft.position.x + 2, topLeft.position.y + 1, "ACTIVE PANELS", OpsAmber);
    buffer.writeString(topRight.position.x + 2, topRight.position.y + 1, "BUILD STATUS", OpsAmber);
    buffer.writeString(bottomLeft.position.x + 2, bottomLeft.position.y + 1, "OBJECT PREVIEW", OpsAmber);
    buffer.writeString(bottomRight.position.x + 2, bottomRight.position.y + 1, "SITUATION SUMMARY", OpsAmber);

    const TextObject banner = makeBanner(
        "Small Shadow.flf",
        "TUI Ops Wall",
        OpsAmber,
        AsciiBanner::ComposeMode::FullWidth);
    writeObject(page, banner, "HeaderContent", Composition::Align::center());

    writeTextBlock(
        page,
        "Build Queue\nAsset Pipeline\nTheme Preview\nFrame Diff\nRenderer Trace\nUnicode Audit",
        "TopLeftContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center));

    const std::string progressBlock =
        std::string("Layout  ") + makeProgressBar(18, pulse(elapsedSeconds(), 0.70), '=') + "\n"
        + "Objects " + makeProgressBar(18, pulse(elapsedSeconds(), 1.10, 0.8), '=') + "\n"
        + "Themes  " + makeProgressBar(18, pulse(elapsedSeconds(), 0.90, 1.6), '=');

    writeTextBlock(
        page,
        progressBlock,
        "TopRightContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center));

    writeObject(page, cubeObject(), "BottomLeftContent", Composition::Align::centerLeft());
    writeObject(page, chipObject(), "BottomLeftContent", Composition::Align::centerRight());

    writeWrapped(
        page,
        "The OpsWall scene is now a standalone screen rather than a timed phase inside one monolithic showcase. That makes it much easier to plug into menus, diagnostics flows, or future demo navigation without carrying the other three layouts around with it.",
        "BottomRightContent",
        makeAlignment(Composition::HorizontalAlign::Left, Composition::VerticalAlign::Center));

    const int footerWidth = std::max(1, page.resolveRegion("FooterContent").size.width);
    writeText(
        page,
        makeMarquee(
            "ops wall  •  four panel command board  •  retained objects  •  figlet banner  •  rgb accents  •  extracted screen class",
            footerWidth,
            static_cast<int>(elapsedSeconds() * 10.0)),
        "FooterContent",
        Composition::Align::centerLeft());

    writeText(page, "SHOWCASE / OPS WALL", "FooterContent", Composition::Align::centerRight());

    for (int x = 3; x < buffer.getWidth() - 3; x += 8)
    {
        const int y = static_cast<int>(2 + std::fmod(elapsedSeconds() * 6.0 + static_cast<double>(x), 6.0));
        if (buffer.inBounds(x, y))
        {
            buffer.writeChar(x, y, '*', OpsCyan);
        }
    }
}
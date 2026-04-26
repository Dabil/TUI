#include "Screens/DigitalRainScreen.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "Core/Rect.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Objects/TextObjectComposer.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Surface.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Utilities/Unicode/UnicodeWidth.h"


// TODO:
//  Add Shedding Glyphs
//  have dead glyphs change color or shape rarely
//  add BrightBlack digital drops

namespace
{
    constexpr char32_t RabbitGlyph = U'\U0001F407';

    constexpr int MinimumScreenWidth = 48;
    constexpr int MinimumScreenHeight = 14;
    constexpr int FooterRows = 2;
    constexpr double PreviewAdvanceIntervalSeconds = 0.08;
    constexpr double MutationIntervalSeconds = 0.065;

    constexpr int MaximumDigitalDrops = 150;
    constexpr int DeadGlyphsSpawnRate = 25;
    constexpr int DeadGlyphBlinkRate = 10;

    struct DeadGlyph
    {
        int x = 0;
        int y = 0;
        char32_t glyph = U' ';
    };

    std::vector<DeadGlyph> m_deadGlyphs;
    double m_deadGlyphSpawnAccumulator = 0.0;
}

std::u32string DigitalRainScreen::buildConsoleGlyphPool()
{
    return std::u32string(
        U"0123456789A₿CDEFGHIJKLMNOPQRSTUVWXYZ"
        U"@#$%&*+=<>"
        U"[]{}()"
        U"█▓▒░"
        U"■□▪▫"
        U"▲▼"
        U"○●"
        U"ΑβϲδεφϑհιյΚλʍƞɸπθʀστυƔѡϰψȥ"
        U"┌┐└┘├┤┬┴┼"
        U"╔╗╚╝╠╣╦╩╬"
        U"│┃─━"
        U"←↑→↓");
}

//        U"♜♞♝♛♚♝♞♜♖♘♗♕♔♗♘♖"
std::u32string DigitalRainScreen::buildTerminalGlyphPool()
{
    return std::u32string(
        U"アァカサタナハマヤャラワガザダバパ"
        U"イィキシチニヒミリヰギジヂビピ"
        U"ウゥクスツヌフムユュルグズブヅプ"
        U"エェケセテネヘメレヱゲゼデベペ"
        U"オォコソトノホモヨョロヲゴゾドボポヴッン"
        U"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" // standard numbers and letters
        U"ΑβϲδεφϑհιյΚλʍƞɸπθʀστυƔѡϰψȥ"           // greek alphabet
        U"♪♫⌘₿äü∄∃ƒ±£µℇ")
        + std::u32string(1, RabbitGlyph);
}

std::u32string DigitalRainScreen::buildGlyphPoolForHost(TerminalHostKind hostKind)
{
    switch (hostKind)
    {
    case TerminalHostKind::ClassicConsoleWindow:
    case TerminalHostKind::RedirectedOrPipe:
    case TerminalHostKind::Unknown:
        return buildConsoleGlyphPool();

    case TerminalHostKind::WindowsTerminal:
    case TerminalHostKind::VsCodeIntegratedTerminal:
    case TerminalHostKind::ConEmu:
    case TerminalHostKind::Mintty:
    case TerminalHostKind::OtherTerminalHost:
    default:
        return buildTerminalGlyphPool();
    }
}

DigitalRainScreen::DigitalRainScreen(TerminalHostKind hostKind)
    : m_hostKind(hostKind)
    , m_glyphPool(buildGlyphPoolForHost(hostKind))
    , m_rng(0x4D415452u)
{
    const Color black = Color::FromBasic(Color::Basic::Black);

    m_headStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(black);

    m_secondStyle =
        style::Fg(Color::FromBasic(Color::Basic::White))
        + style::Bg(black);

    m_thirdStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightGreen))
        + style::Bg(black);

    m_fourthStyle =
        style::Fg(Color::FromBasic(Color::Basic::Green))
        + style::Bg(black);

    m_tailStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(black);

    m_backgroundStyle = style::Bg(black);

    m_titleStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightGreen))
        + style::Bg(black);

    m_labelStyle =
        style::Underline
        + style::Fg(Color::FromBasic(Color::Basic::BrightWhite))
        + style::Bg(black);

    m_previewStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightGreen))
        + style::Bg(black);

    m_borderStyle =
        style::Fg(Color::FromBasic(Color::Basic::Green))
        + style::Bg(black);
}

void DigitalRainScreen::onEnter()
{
    m_elapsedSeconds = 0.0;
    m_previewAdvanceTimer = 0.0;
    m_previewOffset = 0;
    m_streams.clear();

    invalidateStaticUiCache();
    invalidateMinimumScreenUiCache();
}

void DigitalRainScreen::update(double deltaTime)
{
    m_elapsedSeconds += deltaTime;

    if (m_rainWidth <= 0 || m_rainHeight <= 0 || m_streams.empty())
    {
        return;
    }

    updateStreams(deltaTime);
    spawnDeadGlyphs(deltaTime);
    updatePreview(deltaTime);
}

void DigitalRainScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    const int screenWidth = buffer.getWidth();
    const int screenHeight = buffer.getHeight();

    if (screenWidth < MinimumScreenWidth || screenHeight < MinimumScreenHeight)
    {
        ensureMinimumScreenUiCache(screenWidth, screenHeight);
        m_minimumScreenUiObject.draw(buffer, 0, 0, Composition::WritePresets::solidObject());
        return;
    }

    ensureLayout(screenWidth, screenHeight);
    ensureStaticUiCache();

    m_staticUiObject.draw(buffer, 0, 0, Composition::WritePresets::visibleObject());
    drawRain(buffer);

    const TextObject contractPanel = buildUnicodeContractPanelTextObject();
    const int panelX = std::max(4, m_screenWidth - contractPanel.getWidth() - 4);
    contractPanel.draw(buffer, panelX, 2, Composition::WritePresets::solidObject());

    const int footerLabelY = m_screenHeight - 3;
    drawPreviewLine(buffer, 8, footerLabelY, std::max(0, m_screenWidth - 10), m_previewOffset);
}

void DigitalRainScreen::ensureLayout(int screenWidth, int screenHeight)
{
    const int newRainLeft = 3;
    const int newRainTop = 2;
    const int newRainWidth = std::max(0, screenWidth - 7);
    const int newRainHeight = std::max(0, screenHeight - 4 - FooterRows);

    if (screenWidth == m_screenWidth &&
        screenHeight == m_screenHeight &&
        newRainWidth == m_rainWidth &&
        newRainHeight == m_rainHeight)
    {
        return;
    }

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_rainLeft = newRainLeft;
    m_rainTop = newRainTop;
    m_rainWidth = newRainWidth;
    m_rainHeight = newRainHeight;

    invalidateStaticUiCache();
    rebuildStreams();
}

void DigitalRainScreen::ensureStaticUiCache()
{
    const std::u32string titleText = buildTitleText();
    const std::u32string footerDescriptionText = buildFooterDescriptionText();
    const bool consoleFooter = usesConsoleFooter();

    const bool contentChanged =
        (m_cachedTitleText != titleText) ||
        (m_cachedFooterDescriptionText != footerDescriptionText) ||
        (m_cachedConsoleFooter != consoleFooter);

    if (!m_staticUiDirty && !contentChanged)
    {
        return;
    }

    m_cachedTitleText = titleText;
    m_cachedFooterDescriptionText = footerDescriptionText;
    m_cachedConsoleFooter = consoleFooter;
    m_staticUiObject = buildStaticUiTextObject();
    m_staticUiDirty = false;
}

void DigitalRainScreen::ensureMinimumScreenUiCache(int screenWidth, int screenHeight)
{
    if (!m_minimumScreenUiDirty &&
        screenWidth == m_minimumScreenUiWidth &&
        screenHeight == m_minimumScreenUiHeight)
    {
        return;
    }

    m_minimumScreenUiWidth = screenWidth;
    m_minimumScreenUiHeight = screenHeight;
    m_minimumScreenUiObject = buildMinimumScreenTextObject(screenWidth, screenHeight);
    m_minimumScreenUiDirty = false;
}

void DigitalRainScreen::invalidateStaticUiCache()
{
    m_staticUiDirty = true;
}

void DigitalRainScreen::invalidateMinimumScreenUiCache()
{
    m_minimumScreenUiDirty = true;
}

TextObject DigitalRainScreen::buildStaticUiTextObject() const
{
    if (m_screenWidth <= 0 || m_screenHeight <= 0)
    {
        return TextObject();
    }

    TextObjectComposer composer;
    composer.addFilledRect(0, 0, m_screenWidth, m_screenHeight, U' ', m_backgroundStyle, 0, "background");
    composer.addFrame(
        0,
        0,
        m_screenWidth,
        m_screenHeight,
        m_borderStyle,
        ObjectFactory::doubleLineBorder(),
        10,
        "outerFrame");
    composer.addFrame(
        2,
        1,
        m_screenWidth - 4,
        m_screenHeight - 2 - FooterRows,
        m_tailStyle,
        ObjectFactory::singleLineBorder(),
        10,
        "innerFrame");
    composer.addText(buildTitleText(), 4, 0, m_titleStyle, 20, "title");

    const int footerLabelY = m_screenHeight - 3;
    const int footerPreviewY = m_screenHeight - 2;

    composer.addText(U"Pool", 2, footerLabelY, m_labelStyle, 20, "poolLabel");
    composer.addGlyph(U':', 6, footerLabelY, m_headStyle, 20, "poolColon");

    if (usesConsoleFooter())
    {
        composer.addText(U"Sample:", 2, footerPreviewY, m_labelStyle, 20, "sampleLabel");
        composer.addGlyph(U'█', 10, footerPreviewY, m_previewStyle, 20, "sampleGlyph1");
        composer.addGlyph(U'◆', 12, footerPreviewY, m_previewStyle, 20, "sampleGlyph2");
        composer.addGlyph(U'╬', 14, footerPreviewY, m_previewStyle, 20, "sampleGlyph3");
        composer.addGlyph(U'↑', 16, footerPreviewY, m_previewStyle, 20, "sampleGlyph4");
        composer.addGlyph(U'△', 18, footerPreviewY, m_previewStyle, 20, "sampleGlyph5");
        composer.addText(buildFooterDescriptionText(), 21, footerPreviewY, m_labelStyle, 20, "footerDescription");
    }
    else
    {
        composer.addGlyph(U'ア', 10, footerPreviewY, m_previewStyle, 20, "sampleGlyph1");
        composer.addGlyph(U'♔', 12, footerPreviewY, m_previewStyle, 20, "sampleGlyph2");
        composer.addGlyph(U'₿', 14, footerPreviewY, m_previewStyle, 20, "sampleGlyph3");
        composer.addGlyph(U'♪', 16, footerPreviewY, m_previewStyle, 20, "sampleGlyph4");
        composer.addGlyph(RabbitGlyph, 18, footerPreviewY, m_previewStyle, 20, "sampleGlyph5");
        composer.addText(buildFooterDescriptionText(), 21, footerPreviewY, m_labelStyle, 20, "footerDescription");
    }

    TextObjectComposer::BuildTextObjectOptions options;
    options.writePolicy = Composition::WritePresets::solidObject();
    return composer.buildTextObject(options);
}

TextObject DigitalRainScreen::buildMinimumScreenTextObject(int screenWidth, int screenHeight) const
{
    if (screenWidth <= 0 || screenHeight <= 0)
    {
        return TextObject();
    }

    TextObjectComposer composer;
    composer.addFilledRect(0, 0, screenWidth, screenHeight, U' ', m_backgroundStyle, 0, "background");
    composer.addFrame(
        0,
        0,
        screenWidth,
        screenHeight,
        m_borderStyle,
        ObjectFactory::doubleLineBorder(),
        10,
        "outerFrame");
    composer.addTextUtf8(
        "Digital Rain needs a larger console window.",
        2,
        1,
        Themes::Warning,
        20,
        "warningText");
    composer.addTextUtf8(
        "Recommended: at least 48 x 14.",
        2,
        3,
        Themes::Subtitle,
        20,
        "recommendationText");

    TextObjectComposer::BuildTextObjectOptions options;
    options.writePolicy = Composition::WritePresets::solidObject();
    return composer.buildTextObject(options);
}

TextObject DigitalRainScreen::buildUnicodeContractPanelTextObject() const
{
    const std::vector<std::u32string> rows = buildUnicodeContractPanelRows();
    const int panelWidth = 52;
    const int panelHeight = static_cast<int>(rows.size()) + 2;

    TextObjectComposer composer;
    composer.addFilledRect(0, 0, panelWidth, panelHeight, U' ', m_backgroundStyle, 0, "contractPanelBackground");
    composer.addFrame(
        0,
        0,
        panelWidth,
        panelHeight,
        m_tailStyle,
        ObjectFactory::singleLineBorder(),
        10,
        "contractPanelFrame");

    for (std::size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
    {
        const Style& rowStyle = (rowIndex == 0) ? m_labelStyle : m_previewStyle;
        composer.addText(rows[rowIndex], 1, static_cast<int>(rowIndex) + 1, rowStyle, 20, "contractPanelRow");
    }

    TextObjectComposer::BuildTextObjectOptions options;
    options.writePolicy = Composition::WritePresets::solidObject();
    return composer.buildTextObject(options);
}

std::vector<std::u32string> DigitalRainScreen::buildUnicodeContractPanelRows() const
{
    const auto statusPrefix = [](bool passed) -> std::u32string
        {
            return passed ? U"[PASS] " : U"[FAIL] ";
        };

    const TextObject combiningObject = TextObject::fromU32(U"a\u0301");
    const bool combiningPassed =
        combiningObject.getWidth() == 1 &&
        combiningObject.getHeight() == 1 &&
        combiningObject.getCell(0, 0).isGlyph() &&
        combiningObject.getCell(0, 0).cluster == U"a\u0301" &&
        combiningObject.getCell(0, 0).cluster.size() == 2;

    const TextObject wideObject = TextObject::fromU32(U"界");
    const bool widePassed =
        wideObject.getWidth() == 2 &&
        wideObject.getHeight() == 1 &&
        wideObject.getCell(0, 0).isGlyph() &&
        wideObject.getCell(0, 0).width == CellWidth::Two &&
        wideObject.getCell(0, 0).cluster == U"界" &&
        wideObject.getCell(1, 0).isWideTrailing() &&
        wideObject.getCell(1, 0).width == CellWidth::Zero;

    const TextObject spaceObject = TextObject::fromU32(U" ");
    const bool spacePassed =
        spaceObject.getWidth() == 1 &&
        spaceObject.getHeight() == 1 &&
        spaceObject.getCell(0, 0).isGlyph() &&
        !spaceObject.getCell(0, 0).isEmpty() &&
        spaceObject.getCell(0, 0).glyph == U' ' &&
        spaceObject.getCell(0, 0).cluster == U" ";

    const TextObject emptyObject = TextObject::fromU32(U"");
    const bool emptyPassed =
        emptyObject.getWidth() == 0 &&
        emptyObject.isEmpty();

    const TextObject mixedObject = TextObject::fromU32(U"a\u0301界");
    const bool mixedPassed =
        mixedObject.getWidth() == 3 &&
        mixedObject.getHeight() == 1 &&
        mixedObject.getCell(0, 0).isGlyph() &&
        mixedObject.getCell(0, 0).cluster == U"a\u0301" &&
        mixedObject.getCell(1, 0).isGlyph() &&
        mixedObject.getCell(1, 0).width == CellWidth::Two &&
        mixedObject.getCell(1, 0).cluster == U"界" &&
        mixedObject.getCell(2, 0).isWideTrailing();

    std::vector<std::u32string> rows;
    rows.push_back(U"UNICODE CONTRACT CHECKS");
    rows.push_back(statusPrefix(combiningPassed) + U"Combining mark preserved");
    rows.push_back(U"       input: a + U+0301");
    rows.push_back(U"       expected: one cell with cluster á");
    rows.push_back(statusPrefix(widePassed) + U"Wide glyph creates trail");
    rows.push_back(U"       input: 界");
    rows.push_back(U"       expected: Glyph + WideTrail");
    rows.push_back(statusPrefix(spacePassed) + U"Authored space is not Empty");
    rows.push_back(U"       input: \" \"");
    rows.push_back(U"       expected: Glyph U' '");
    rows.push_back(statusPrefix(emptyPassed) + U"Empty stays transparent");
    rows.push_back(U"       expected: no authored cell");
    rows.push_back(statusPrefix(mixedPassed) + U"Cluster boundaries preserved");
    rows.push_back(U"       input: a + U+0301 + 界");
    rows.push_back(U"       expected: cluster, wide glyph, trail");
    return rows;
}

std::u32string DigitalRainScreen::buildTitleText() const
{
    return U"[ Digital Rain Unicode Validation ]";
}

std::u32string DigitalRainScreen::buildFooterDescriptionText() const
{
    if (usesConsoleFooter())
    {
        return U"Console-safe fallback glyphs active";
    }

    return U"Katakana + chess + music + rabbit + math";
}

bool DigitalRainScreen::usesConsoleFooter() const
{
    switch (m_hostKind)
    {
    case TerminalHostKind::ClassicConsoleWindow:
    case TerminalHostKind::RedirectedOrPipe:
    case TerminalHostKind::Unknown:
        return true;

    case TerminalHostKind::WindowsTerminal:
    case TerminalHostKind::VsCodeIntegratedTerminal:
    case TerminalHostKind::ConEmu:
    case TerminalHostKind::Mintty:
    case TerminalHostKind::OtherTerminalHost:
    default:
        return false;
    }
}

void DigitalRainScreen::rebuildStreams()
{
    if (m_rainWidth <= 0 || m_rainHeight <= 0)
    {
        m_streams.clear();
        return;
    }

    m_streams.assign(static_cast<std::size_t>(m_rainWidth), Stream{});

    for (Stream& stream : m_streams)
    {
        resetStream(stream, true);
    }
}

void DigitalRainScreen::resetStream(Stream& stream, bool staggerStart)
{
    stream.active = false;
    stream.headY = 0.0f;
    stream.speed = 0.0f;
    stream.length = 0;
    stream.mutationTimer = 0.0;
    stream.glyphs.clear();
    stream.composition = DropComposition::GreenOnly;

    std::uniform_real_distribution<double> delayDistribution(
        0.0,
        staggerStart ? 4.8 : 2.5);

    stream.respawnDelay = delayDistribution(m_rng);

    if (!staggerStart)
    {
        configureActiveStream(stream);
    }
}

void DigitalRainScreen::configureActiveStream(Stream& stream)
{
    std::uniform_real_distribution<float> speedDistribution(0.25f, 28.0f);
    std::uniform_int_distribution<int> lengthDistribution(6, std::max(6, std::min(m_rainHeight, 24)));
    std::uniform_real_distribution<float> headOffsetDistribution(1.0f, static_cast<float>(std::max(2, m_rainHeight / 2)));
    std::uniform_int_distribution<int> compositionDistribution(0, 99);

    stream.active = true;
    stream.speed = speedDistribution(m_rng);
    stream.length = lengthDistribution(m_rng);
    stream.headY = -headOffsetDistribution(m_rng);
    stream.mutationTimer = 0.0;
    stream.glyphs.assign(static_cast<std::size_t>(stream.length), U' ');

    const int compositionRoll = compositionDistribution(m_rng);

    if (compositionRoll < 25)
    {
        stream.composition = DropComposition::BrightWhiteWhite;
    }
    else if (compositionRoll < 50)
    {
        stream.composition = DropComposition::WhiteOnly;
    }
    else
    {
        stream.composition = DropComposition::GreenOnly;
    }

    for (char32_t& glyph : stream.glyphs)
    {
        glyph = randomGlyph();
    }
}

void DigitalRainScreen::updateStreams(double deltaTime)
{
    std::uniform_int_distribution<int> mutationCountDistribution(1, 3);

    for (Stream& stream : m_streams)
    {
        if (!stream.active)
        {
            stream.respawnDelay -= deltaTime;

            if (stream.respawnDelay <= 0.0)
            {
                // Enforce global active stream cap
                if (countActiveStreams() < MaximumDigitalDrops)
                {
                    configureActiveStream(stream);
                }
                else
                {
                    // Defer activation slightly to avoid tight retry loop
                    stream.respawnDelay = 0.1;
                }
            }

            continue;
        }

        const int previousHeadRow = static_cast<int>(std::floor(stream.headY));
        stream.headY += stream.speed * static_cast<float>(deltaTime);
        const int currentHeadRow = static_cast<int>(std::floor(stream.headY));

        for (int row = previousHeadRow; row < currentHeadRow; ++row)
        {
            if (!stream.glyphs.empty())
            {
                for (int i = stream.length - 1; i > 0; --i)
                {
                    stream.glyphs[static_cast<std::size_t>(i)] = stream.glyphs[static_cast<std::size_t>(i - 1)];
                }

                stream.glyphs[0] = randomGlyph();
            }
        }

        stream.mutationTimer += deltaTime;

        while (stream.mutationTimer >= MutationIntervalSeconds)
        {
            stream.mutationTimer -= MutationIntervalSeconds;

            const int mutationCount = mutationCountDistribution(m_rng);
            if (!stream.glyphs.empty())
            {
                std::uniform_int_distribution<int> glyphIndexDistribution(0, static_cast<int>(stream.glyphs.size()) - 1);

                for (int i = 0; i < mutationCount; ++i)
                {
                    stream.glyphs[static_cast<std::size_t>(glyphIndexDistribution(m_rng))] = randomGlyph();
                }
            }
        }

        if (stream.headY - static_cast<float>(stream.length) > static_cast<float>(m_rainHeight))
        {
            resetStream(stream, false);
        }
    }
}

void DigitalRainScreen::updatePreview(double deltaTime)
{
    if (m_glyphPool.empty())
    {
        return;
    }

    m_previewAdvanceTimer += deltaTime;

    while (m_previewAdvanceTimer >= PreviewAdvanceIntervalSeconds)
    {
        m_previewAdvanceTimer -= PreviewAdvanceIntervalSeconds;
        m_previewOffset = (m_previewOffset + 1) % static_cast<int>(m_glyphPool.size());
    }
}

int DigitalRainScreen::countActiveStreams() const
{
    int count = 0;

    for (const Stream& stream : m_streams)
    {
        if (stream.active)
        {
            ++count;
        }
    }

    return count;
}

void DigitalRainScreen::spawnDeadGlyphs(double deltaTime)
{
    // Accumulate time
    m_deadGlyphSpawnAccumulator += deltaTime;

    // How many glyphs per second
    constexpr double spawnRate = static_cast<double>(DeadGlyphsSpawnRate);

    if (spawnRate <= 0.0)
    {
        return;
    }

    const double interval = 1.0 / spawnRate;

    // Spawn as many as needed this frame
    while (m_deadGlyphSpawnAccumulator >= interval)
    {
        m_deadGlyphSpawnAccumulator -= interval;

        if (m_rainWidth <= 0 || m_rainHeight <= 0)
        {
            continue;
        }

        std::uniform_int_distribution<int> xDist(0, m_rainWidth - 1);
        std::uniform_int_distribution<int> yDist(0, m_rainHeight - 1);

        DeadGlyph glyph;
        glyph.x = xDist(m_rng);
        glyph.y = yDist(m_rng);
        glyph.glyph = randomGlyph();

        m_deadGlyphs.push_back(glyph);
    }

    // Optional: cap to prevent unbounded growth
    constexpr std::size_t maxDeadGlyphs = 512;
    if (m_deadGlyphs.size() > maxDeadGlyphs)
    {
        m_deadGlyphs.erase(
            m_deadGlyphs.begin(),
            m_deadGlyphs.begin() + (m_deadGlyphs.size() - maxDeadGlyphs));
    }
}

void DigitalRainScreen::drawRain(ScreenBuffer& buffer) const
{
    buffer.fillRect(
        Rect{ Point{ m_rainLeft, m_rainTop }, Size{ m_rainWidth, m_rainHeight } },
        U' ',
        m_backgroundStyle);

    // Draw dead glyphs (background noise layer)
    for (const DeadGlyph& glyph : m_deadGlyphs)
    {
        buffer.writeCodePoint(
            m_rainLeft + glyph.x,
            m_rainTop + glyph.y,
            glyph.glyph,
            m_tailStyle); // dim / "dead"
    }

    for (int column = 0; column < m_rainWidth; ++column)
    {
        const Stream& stream = m_streams[static_cast<std::size_t>(column)];
        if (!stream.active)
        {
            continue;
        }

        const int headRow = static_cast<int>(std::floor(stream.headY));

        for (int trailIndex = 0; trailIndex < stream.length; ++trailIndex)
        {
            const int y = headRow - trailIndex;

            if (y < 0 || y >= m_rainHeight)
            {
                continue;
            }

            const char32_t glyph = stream.glyphs[static_cast<std::size_t>(trailIndex)];
            const Style style = styleForTrailIndex(trailIndex, stream.length, stream.composition);
            buffer.writeCodePoint(m_rainLeft + column, m_rainTop + y, glyph, style);
        }
    }
}

void DigitalRainScreen::drawPreviewLine(ScreenBuffer& buffer, int x, int y, int availableWidth, int startIndex) const
{
    if (availableWidth <= 0 || m_glyphPool.empty())
    {
        return;
    }

    int writeX = x;
    int glyphIndex = startIndex % static_cast<int>(m_glyphPool.size());

    while (writeX < x + availableWidth)
    {
        const char32_t glyph = m_glyphPool[static_cast<std::size_t>(glyphIndex)];
        const CellWidth width = UnicodeWidth::measureCodePointWidth(glyph);
        const int glyphWidth = (width == CellWidth::Two) ? 2 : 1;

        if (writeX + glyphWidth > x + availableWidth)
        {
            break;
        }

        buffer.writeCodePoint(writeX, y, glyph, m_previewStyle);
        writeX += glyphWidth;
        glyphIndex = (glyphIndex + 1) % static_cast<int>(m_glyphPool.size());
    }
}

char32_t DigitalRainScreen::randomGlyph()
{
    if (m_glyphPool.empty())
    {
        return U' ';
    }

    std::uniform_int_distribution<int> glyphDistribution(0, static_cast<int>(m_glyphPool.size()) - 1);
    return m_glyphPool[static_cast<std::size_t>(glyphDistribution(m_rng))];
}

Style DigitalRainScreen::styleForTrailIndex(
    int trailIndex,
    int streamLength,
    DropComposition composition) const
{
    if (streamLength <= 0)
    {
        return m_tailStyle;
    }

    const int brightBlackStart = std::max(0, streamLength - 2);
    if (trailIndex >= brightBlackStart)
    {
        return m_tailStyle;
    }

    int brightGreenStart = 0;
    int greenStart = 0;

    switch (composition)
    {
    case DropComposition::BrightWhiteWhite:
        // 1 BrightWhite
        // 1 White
        // half middle BrightGreen
        // half middle Green
        // last 2 BrightBlack
        if (trailIndex == 0)
        {
            return m_headStyle;
        }

        if (trailIndex == 1)
        {
            return m_secondStyle;
        }

        brightGreenStart = 2;
        break;

    case DropComposition::WhiteOnly:
        // 1 White
        // half middle BrightGreen
        // half middle Green
        // last 2 BrightBlack
        if (trailIndex == 0)
        {
            return m_secondStyle;
        }

        brightGreenStart = 1;
        break;

    case DropComposition::GreenOnly:
    default:
        // half BrightGreen
        // half Green
        // last 2 BrightBlack
        brightGreenStart = 0;
        break;
    }

    const int middleCount = std::max(0, brightBlackStart - brightGreenStart);
    if (middleCount <= 0)
    {
        return m_fourthStyle;
    }

    const int brightGreenCount = (middleCount + 1) / 2;
    greenStart = brightGreenStart + brightGreenCount;

    if (trailIndex < greenStart)
    {
        return m_thirdStyle;
    }

    return m_fourthStyle;
}

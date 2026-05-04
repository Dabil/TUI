#include "Screens/DigitalRainScreen.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "Core/Rect.h"
#include "Rendering/Composition/WritePresets.h"
#include "Rendering/Composition/PageComposer.h"
#include "Rendering/Objects/TextObjectComposer.h"
#include "Rendering/Objects/TextObjectFactory.h"
#include "Rendering/Objects/TextObjectExporter.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Surface.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Utilities/Unicode/UnicodeWidth.h"
#include "Utilities/Unicode/UnicodeConversion.h"


// TODO:
//  Add Shedding Glyphs
//  have dead glyphs change color or shape rarely
//  add BrightBlack digital drops

namespace
{
    using Composition::Alignment;
    using Composition::PageComposer;

    constexpr int MinimumScreenWidth = 48;
    constexpr int MinimumScreenHeight = 14;
    constexpr int FooterRows = 2;
    constexpr double PreviewAdvanceIntervalSeconds = 0.08;
    constexpr char32_t RabbitGlyph = U'\U0001F407';
}

rainXRay::rainXRay()
{
    m_xRayBoxStyle =
          style::Fg(Color::FromBasic(Color::Basic::Green))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    m_xRayPanelStyle =
          style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    m_xRayX = -100;
    m_xRayY = -100;

    // cells per second
    m_xRayDx = 2.0f;
    m_xRayDy = 2.0f;

    m_TopBounds = 1;
    m_BottomBounds = 3;
    m_LeftBounds = 2;
    m_RightBounds = 2;
    
    m_xRayWidth  = 0;
    m_xRayHeight = 0;
}

rainXRay::~rainXRay()
{
    m_RainXRayBox.clear();
    m_RainXRayPanel.clear();
}

void rainXRay::draw(ScreenBuffer& buffer)
{
    m_RainXRayBox.draw(
        buffer,
        static_cast<int>(m_xRayX),
        static_cast<int>(m_xRayY),
        Composition::WritePresets::visibleObject());

    m_RainXRayPanel.draw(
        buffer,
        static_cast<int>(m_xRayX) + 1,
        static_cast<int>(m_xRayY) + 1,
        Composition::WritePresets::styleBlock());
}

void rainXRay::move(int screenWidth, int screenHeight, double deltaTime)
{
    if (!m_xRay_initialized || screenWidth <= 0 || screenHeight <= 0)
    {
        return;
    }

    const float minX = static_cast<float>(m_LeftBounds);
    const float maxX = static_cast<float>(screenWidth - m_RightBounds - m_xRayWidth);
    const float minY = static_cast<float>(m_TopBounds);
    const float maxY = static_cast<float>(screenHeight - m_BottomBounds - m_xRayHeight);

    if (maxX <= minX || maxY <= minY)
    {
        return;
    }

    m_xRayX += m_xRayDx * static_cast<float>(deltaTime);
    m_xRayY += m_xRayDy * static_cast<float>(deltaTime);

    if (m_xRayX <= minX || m_xRayX >= maxX)
    {
        m_xRayX = std::clamp(m_xRayX, minX, maxX);
        m_xRayDx *= -1.0f;
    }

    if (m_xRayY <= minY || m_xRayY >= maxY)
    {
        m_xRayY = std::clamp(m_xRayY, minY, maxY);
        m_xRayDy *= -1.0f;
    }
}

void rainXRay::reset(int screenWidth, int screenHeight)
{
    m_xRayWidth = screenWidth / 4;
    m_xRayHeight = screenHeight / 4;

    m_xRayX = (screenWidth - m_xRayWidth) / 2;
    m_xRayY = (screenHeight - m_xRayHeight) / 2;

    TextObjectComposer composer;
    composer.addSolidFrame(
        0,
        0,
        m_xRayWidth,
        m_xRayHeight,
        m_xRayBoxStyle,
        ObjectFactory::roundedBorder(),
        10,
        "outerFrame");
    composer.addText(U"[DigiX-Ray]", (m_xRayWidth / 2) - 5, 0, m_xRayBoxStyle, 20, "title");
    m_RainXRayBox = composer.buildTextObject();

    m_RainXRayPanel = ObjectFactory::makeFilledRect(
        m_xRayWidth - 2,
        m_xRayHeight - 2,
        U' ',
        m_xRayPanelStyle);

    m_xRay_initialized = true;
}

void rainXRay::invalidateXray()
{
    m_xRay_initialized = false;
}

bool rainXRay::initialized()
{
    return m_xRay_initialized;
}

DigitalRainScreen::DigitalRainScreen(TerminalHostKind hostKind)
    : m_hostKind(hostKind)
    , m_DigitalRain(hostKind)
{
    const Color black = Color::FromBasic(Color::Basic::Black);

    m_backgroundStyle = style::Bg(black);

    m_titleStyle =
        style::Bold
        + style::Fg(Color::FromBasic(Color::Basic::White))
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

    m_windowStyle =
        style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
        + style::Bg(black);

    m_backgroundStyle = style::Bg(black);
}

void DigitalRainScreen::onEnter()
{
    m_elapsedSeconds = 0.0;
    m_previewAdvanceTimer = 0.0;
    m_previewOffset = 0;

    invalidateStaticUiCache();
    invalidateMinimumScreenUiCache();
    m_xRayMachine.invalidateXray();
    m_DigitalRain.onEnter();
}

void DigitalRainScreen::update(double deltaTime)
{
    m_elapsedSeconds += deltaTime;
    updatePreview(deltaTime);
    m_xRayMachine.move(m_screenWidth, m_screenHeight, deltaTime);
    m_DigitalRain.update(deltaTime);
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

    PageComposer page(buffer);
    page.clearRegions();

    page.createFullScreenRegion("Screen");
    page.createInsetRegion("DigitalRain", "Screen", 3, 2, 3, 3);

    const Rect& viewport = page.resolveRegion("DigitalRain");
    m_DigitalRain.draw(surface, viewport);

    m_staticUiObject.draw(buffer, 0, 0, Composition::WritePresets::visibleObject());

    const int footerLabelY = m_screenHeight - 3;
    drawPreviewLine(buffer, 8, footerLabelY, std::max(0, m_screenWidth - 10), m_previewOffset);

    const TextObject contractPanel = buildUnicodeContractPanelTextObject();
    const int panelX = std::max(4, m_screenWidth - contractPanel.getWidth() - 4);
    contractPanel.draw(buffer, panelX, 2, Composition::WritePresets::solidObject());

    if (!m_xRayMachine.initialized())
    {
        m_xRayMachine.reset(screenWidth, screenHeight);
    }

    m_xRayMachine.draw(buffer);
}

void DigitalRainScreen::ensureLayout(int screenWidth, int screenHeight)
{
    if (screenWidth == m_screenWidth && screenHeight == m_screenHeight)
    {
        return;
    }

    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    invalidateStaticUiCache();
    m_xRayMachine.invalidateXray();
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
        m_windowStyle,
        ObjectFactory::singleLineBorder(),
        10,
        "innerFrame");
    composer.addText(buildTitleText(), 4, 0, m_titleStyle, 20, "title");

    const int footerLabelY = m_screenHeight - 3;
    const int footerPreviewY = m_screenHeight - 2;

    composer.addText(U"Pool", 2, footerLabelY, m_labelStyle, 20, "poolLabel");
    composer.addGlyph(U':', 6, footerLabelY, m_labelStyle, 20, "poolColon");

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
        m_windowStyle,
        ObjectFactory::singleLineBorder(),
        10,
        "contractPanelFrame");

    std::u32string H1 = U"UNICODE CONTRACT CHECKS";
    std::u32string H2 = U"UTF-8 OUTPUT CHECKS";
    std::u32string H3 = U"STRESS ROUND-TRIP CHECK";

    for (std::size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
    {
        const Style& rowStyle = (rows[rowIndex] == H1 || rows[rowIndex] == H2 || rows[rowIndex] == H3) ? m_labelStyle : m_previewStyle;
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

    const auto byteCountText = [](const std::string& text) -> std::u32string
        {
            return UnicodeConversion::utf8ToU32(std::to_string(text.size()));
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

    const std::u32string snapshotText = U"e\u0301 表 ✅ √";
    const std::string expectedSnapshotUtf8 = UnicodeConversion::u32ToUtf8(snapshotText);

    const TextObject snapshotObject = TextObject::fromU32(snapshotText);

    ScreenBuffer snapshotBuffer(
        snapshotObject.getWidth(),
        snapshotObject.getHeight());

    snapshotObject.draw(
        snapshotBuffer,
        0,
        0,
        Composition::WritePresets::solidObject());

    const std::string actualSnapshotUtf8 = snapshotBuffer.renderToUtf8String();

    const bool screenBufferSnapshotPassed =
        actualSnapshotUtf8 == expectedSnapshotUtf8;

    TextObjectExporter::SaveOptions exportOptions;
    exportOptions.fileType = TextObjectExporter::FileType::Txt;
    exportOptions.encoding = TextObjectExporter::Encoding::Utf8;
    exportOptions.lineEnding = TextObjectExporter::LineEnding::Lf;
    exportOptions.includeUtf8Bom = false;
    exportOptions.preserveTrailingSpaces = true;

    const TextObjectExporter::SaveResult exportResult =
        TextObjectExporter::exportToBytes(snapshotObject, exportOptions);

    const bool textObjectExportPassed =
        exportResult.success &&
        exportResult.bytes == expectedSnapshotUtf8;

    const bool snapshotMatchesExport =
        exportResult.success &&
        actualSnapshotUtf8 == exportResult.bytes;

    const std::u32string stressText =
        U"A"
        U"e\u0301"
        U" "
        U"界"
        U" "
        U"✅"
        U" "
        U"é é é é"
        U" "
        U"界界界界"
        U" "
        U"✅ ✅ ✅"
        U" "
        U"👍🏽 👍🏽 👍🏽"
        U" "
        U"👨‍👩‍👧‍👦 👨‍👩‍👧‍👦"
        U" "
        U"√"
        U" "
        U"👍🏽"
        U" "
        U"👨‍👩‍👧‍👦"
        U" "
        U"🇺🇸";

    const std::string expectedStressUtf8 =
        UnicodeConversion::u32ToUtf8(stressText);

    const TextObject stressObject = TextObject::fromU32(stressText);

    ScreenBuffer stressBuffer(
        stressObject.getWidth(),
        stressObject.getHeight());

    stressObject.draw(
        stressBuffer,
        0,
        0,
        Composition::WritePresets::solidObject());

    const std::string actualStressUtf8 =
        stressBuffer.renderToUtf8String();

    const TextObjectExporter::SaveResult stressExportResult =
        TextObjectExporter::exportToBytes(stressObject, exportOptions);

    const bool stressRoundTripPassed =
        actualStressUtf8 == expectedStressUtf8 &&
        stressExportResult.success &&
        stressExportResult.bytes == expectedStressUtf8 &&
        actualStressUtf8 == stressExportResult.bytes;

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

    rows.push_back(U"UTF-8 OUTPUT CHECKS");
    rows.push_back(statusPrefix(screenBufferSnapshotPassed) + U"ScreenBuffer emits UTF-8");
    rows.push_back(statusPrefix(textObjectExportPassed) + U"TextObject export emits UTF-8");
    rows.push_back(statusPrefix(snapshotMatchesExport) + U"Snapshot matches export");

    rows.push_back(U"STRESS ROUND-TRIP CHECK");
    rows.push_back(statusPrefix(stressRoundTripPassed) + U"Clusters, wide, emoji, spaces");

    if (!stressRoundTripPassed)
    {
        rows.push_back(U"       expected bytes: " + byteCountText(expectedStressUtf8));
        rows.push_back(U"       snapshot bytes: " + byteCountText(actualStressUtf8));

        if (stressExportResult.success)
        {
            rows.push_back(U"       export bytes:   " + byteCountText(stressExportResult.bytes));
        }
        else
        {
            rows.push_back(U"       export failed");
        }
    }

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



void DigitalRainScreen::updatePreview(double deltaTime)
{
    std::u32string glyphPool = m_DigitalRain.getGlyphPool();

    if (glyphPool.empty())
    {
        return;
    }

    m_previewAdvanceTimer += deltaTime;

    while (m_previewAdvanceTimer >= PreviewAdvanceIntervalSeconds)
    {
        m_previewAdvanceTimer -= PreviewAdvanceIntervalSeconds;
        m_previewOffset = (m_previewOffset + 1) % static_cast<int>(glyphPool.size());
    }
}

void DigitalRainScreen::drawPreviewLine(ScreenBuffer& buffer, int x, int y, int availableWidth, int startIndex) const
{
    std::u32string glyphPool = m_DigitalRain.getGlyphPool();

    if (availableWidth <= 0 || glyphPool.empty())
    {
        return;
    }

    int writeX = x;
    int glyphIndex = startIndex % static_cast<int>(glyphPool.size());

    while (writeX < x + availableWidth)
    {
        const char32_t glyph = glyphPool[static_cast<std::size_t>(glyphIndex)];
        const CellWidth width = UnicodeWidth::measureCodePointWidth(glyph);
        const int glyphWidth = (width == CellWidth::Two) ? 2 : 1;

        if (writeX + glyphWidth > x + availableWidth)
        {
            break;
        }

        buffer.writeCodePoint(writeX, y, glyph, m_previewStyle);
        writeX += glyphWidth;
        glyphIndex = (glyphIndex + 1) % static_cast<int>(glyphPool.size());
    }
}
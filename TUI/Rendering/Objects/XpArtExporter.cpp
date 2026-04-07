#include "Rendering/Objects/XpArtExporter.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>

#include "Rendering/Capabilities/ColorSupport.h"
#include "Rendering/Objects/Cp437Encoding.h"
#include "Rendering/Styles/ColorMapping.h"
#include "Rendering/Styles/ColorResolver.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/ThemeColor.h"
#include "Utilities/Unicode/UnicodeConversion.h"

namespace
{
    using namespace TextObjectExporter;

    constexpr std::uint8_t kGzipId1 = 0x1F;
    constexpr std::uint8_t kGzipId2 = 0x8B;
    constexpr std::uint8_t kGzipCompressionMethodDeflate = 8;

    XpArtExporter::XpColor makeColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
    {
        XpArtExporter::XpColor color;
        color.red = red;
        color.green = green;
        color.blue = blue;
        return color;
    }

    XpArtExporter::XpColor defaultForegroundColor()
    {
        return makeColor(255, 255, 255);
    }

    XpArtExporter::XpColor defaultBackgroundColor()
    {
        return makeColor(0, 0, 0);
    }

    std::string toHexCodePoint(char32_t codePoint)
    {
        std::ostringstream stream;
        stream << "U+"
            << std::uppercase
            << std::hex
            << std::setw(4)
            << std::setfill('0')
            << static_cast<std::uint32_t>(UnicodeConversion::sanitizeCodePoint(codePoint));
        return stream.str();
    }

    void addWarningOnce(
        SaveResult& result,
        SaveWarningCode code,
        const std::string& message)
    {
        for (const SaveWarning& warning : result.warnings)
        {
            if (warning.code == code)
            {
                return;
            }
        }

        SaveWarning warning;
        warning.code = code;
        warning.message = message;
        result.warnings.push_back(warning);
    }

    void addDetailedWarning(
        SaveResult& result,
        SaveWarningCode code,
        const std::string& message,
        const SourcePosition& position,
        char32_t sourceCodePoint,
        char32_t replacementCodePoint)
    {
        SaveWarning warning;
        warning.code = code;
        warning.message = message;
        warning.sourcePosition = position;
        warning.sourceCodePoint = sourceCodePoint;
        warning.replacementCodePoint = replacementCodePoint;
        result.warnings.push_back(warning);
    }

    std::size_t checkedCellCount(int width, int height, bool& ok)
    {
        ok = false;

        if (width <= 0 || height <= 0)
        {
            return 0;
        }

        const std::uint64_t count =
            static_cast<std::uint64_t>(width) *
            static_cast<std::uint64_t>(height);

        if (count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        {
            return 0;
        }

        ok = true;
        return static_cast<std::size_t>(count);
    }

    int layerIndex(int x, int y, int height)
    {
        return (x * height) + y;
    }

    void appendLe16(std::string& bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<char>(value & 0xFF));
        bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
    }

    void appendLe32(std::string& bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<char>(value & 0xFF));
        bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 16) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 24) & 0xFF));
    }

    std::uint32_t crc32ForBytes(std::string_view bytes)
    {
        std::uint32_t crc = 0xFFFFFFFFu;

        for (unsigned char byte : bytes)
        {
            crc ^= static_cast<std::uint32_t>(byte);

            for (int bit = 0; bit < 8; ++bit)
            {
                const bool carry = (crc & 1u) != 0u;
                crc >>= 1;
                if (carry)
                {
                    crc ^= 0xEDB88320u;
                }
            }
        }

        return ~crc;
    }

    bool gzipStoreOnly(std::string_view payload, std::string& outBytes)
    {
        outBytes.clear();
        outBytes.reserve(payload.size() + 32 + ((payload.size() / 65535u) * 5u));

        outBytes.push_back(static_cast<char>(kGzipId1));
        outBytes.push_back(static_cast<char>(kGzipId2));
        outBytes.push_back(static_cast<char>(kGzipCompressionMethodDeflate));
        outBytes.push_back(static_cast<char>(0));
        appendLe32(outBytes, 0);
        outBytes.push_back(static_cast<char>(0));
        outBytes.push_back(static_cast<char>(255));

        std::size_t offset = 0;
        while (offset < payload.size())
        {
            const std::size_t remaining = payload.size() - offset;
            const std::uint16_t blockSize = static_cast<std::uint16_t>(std::min<std::size_t>(remaining, 65535u));
            const bool isFinalBlock = (offset + blockSize) == payload.size();

            outBytes.push_back(static_cast<char>(isFinalBlock ? 0x01 : 0x00));
            appendLe16(outBytes, blockSize);
            appendLe16(outBytes, static_cast<std::uint16_t>(~blockSize));
            outBytes.append(payload.data() + offset, blockSize);

            offset += blockSize;
        }

        const std::uint32_t crc = crc32ForBytes(payload);
        appendLe32(outBytes, crc);
        appendLe32(outBytes, static_cast<std::uint32_t>(payload.size() & 0xFFFFFFFFu));
        return true;
    }

    XpArtExporter::XpColor colorToXpColor(const Color& color)
    {
        const ColorMapping::RgbValue rgb = ColorMapping::toRgb(color);
        return makeColor(rgb.red, rgb.green, rgb.blue);
    }

    bool resolveColorToXp(
        const std::optional<Style::StyleColorValue>& authored,
        const std::optional<Color>& concreteFallback,
        const std::optional<ThemeColor>& themeFallback,
        const XpArtExporter::XpColor& defaultColor,
        bool& outUsedTheme,
        XpArtExporter::XpColor& outColor)
    {
        outUsedTheme = false;

        if (authored.has_value())
        {
            if (authored->hasThemeColor())
            {
                outUsedTheme = true;
            }

            const Color resolved = ColorResolver::resolve(*authored, ColorSupport::Rgb24);
            if (resolved.isDefault())
            {
                outColor = defaultColor;
                return true;
            }

            outColor = colorToXpColor(resolved);
            return true;
        }

        if (concreteFallback.has_value())
        {
            const Color resolved = ColorMapping::mapToSupport(*concreteFallback, ColorSupport::Rgb24);
            if (resolved.isDefault())
            {
                outColor = defaultColor;
                return true;
            }

            outColor = colorToXpColor(resolved);
            return true;
        }

        if (themeFallback.has_value())
        {
            outUsedTheme = true;
            const Color resolved = ColorResolver::resolveThemeColor(*themeFallback, ColorSupport::Rgb24);
            if (resolved.isDefault())
            {
                outColor = defaultColor;
                return true;
            }

            outColor = colorToXpColor(resolved);
            return true;
        }

        outColor = defaultColor;
        return true;
    }

    struct CellStyleResolution
    {
        XpArtExporter::XpColor foreground = defaultForegroundColor();
        XpArtExporter::XpColor background = defaultBackgroundColor();

        bool usedThemeColor = false;
        bool approximatedReverse = false;
        bool approximatedInvisible = false;
        bool droppedUnsupportedStyle = false;
    };

    CellStyleResolution resolveCellStyle(const std::optional<Style>& style)
    {
        CellStyleResolution result;

        if (!style.has_value() || style->isEmpty())
        {
            return result;
        }

        bool usedForegroundTheme = false;
        bool usedBackgroundTheme = false;

        resolveColorToXp(
            style->foregroundColorValue(),
            style->foreground(),
            style->foregroundThemeColor(),
            defaultForegroundColor(),
            usedForegroundTheme,
            result.foreground);

        resolveColorToXp(
            style->backgroundColorValue(),
            style->background(),
            style->backgroundThemeColor(),
            defaultBackgroundColor(),
            usedBackgroundTheme,
            result.background);

        result.usedThemeColor = usedForegroundTheme || usedBackgroundTheme;

        if (style->reverse())
        {
            std::swap(result.foreground, result.background);
            result.approximatedReverse = true;
        }

        if (style->invisible())
        {
            result.foreground = result.background;
            result.approximatedInvisible = true;
        }

        if (style->bold() ||
            style->dim() ||
            style->underline() ||
            style->slowBlink() ||
            style->fastBlink() ||
            style->strike())
        {
            result.droppedUnsupportedStyle = true;
        }

        return result;
    }

    struct GlyphConversionStats
    {
        std::size_t fallbackCount = 0;
        std::size_t replacementCount = 0;

        bool hasFallbackExample = false;
        SourcePosition fallbackPosition;
        char32_t fallbackSourceCodePoint = U'\0';
        char32_t fallbackReplacementCodePoint = U'\0';
        std::string fallbackReason;

        bool hasReplacementExample = false;
        SourcePosition replacementPosition;
        char32_t replacementSourceCodePoint = U'\0';
        char32_t replacementGlyph = U'\0';
    };

    bool convertGlyphToXpCell(
        const TextObjectCell& sourceCell,
        int x,
        int y,
        const SaveOptions& options,
        SaveResult& ioResult,
        GlyphConversionStats& ioStats,
        XpArtExporter::XpCell& outCell)
    {
        char32_t glyph = U' ';
        if (sourceCell.kind == CellKind::Glyph)
        {
            glyph = sourceCell.glyph;
        }

        Cp437Encoding::EncodeResult encodedGlyph;
        if (!Cp437Encoding::tryEncodeWithFallback(glyph, options.xpReplacementGlyph, encodedGlyph))
        {
            ioResult.hasEncodingFailure = true;
            ioResult.firstFailingCodePoint = glyph;
            ioResult.firstFailingPosition = { x, y };
            ioResult.errorMessage =
                "XP export fallback configuration is invalid because SaveOptions::xpReplacementGlyph is not representable in CP437.";
            return false;
        }

        outCell.glyph = static_cast<std::uint32_t>(encodedGlyph.byte);

        if (encodedGlyph.kind == Cp437Encoding::ConversionKind::Exact)
        {
            return true;
        }

        ioResult.hadLossyConversion = true;
        ++ioResult.lossyCodePointCount;

        if (!hasWarning(ioResult, SaveWarningCode::LossyConversionOccurred))
        {
            addWarningOnce(
                ioResult,
                SaveWarningCode::LossyConversionOccurred,
                "XP export performed one or more lossy glyph conversions while mapping Unicode glyphs to CP437 bytes.");
        }

        if (encodedGlyph.kind == Cp437Encoding::ConversionKind::FallbackApproximation)
        {
            ++ioStats.fallbackCount;
            if (!ioStats.hasFallbackExample)
            {
                ioStats.hasFallbackExample = true;
                ioStats.fallbackPosition = { x, y };
                ioStats.fallbackSourceCodePoint = glyph;
                ioStats.fallbackReplacementCodePoint = encodedGlyph.outputCodePoint;
                ioStats.fallbackReason = encodedGlyph.reason != nullptr ? encodedGlyph.reason : "fallback approximation";
            }
            return true;
        }

        ++ioStats.replacementCount;
        if (!ioStats.hasReplacementExample)
        {
            ioStats.hasReplacementExample = true;
            ioStats.replacementPosition = { x, y };
            ioStats.replacementSourceCodePoint = glyph;
            ioStats.replacementGlyph = encodedGlyph.outputCodePoint;
        }

        return true;
    }

    bool convertTextObjectCell(
        const TextObjectCell& sourceCell,
        int x,
        int y,
        const SaveOptions& options,
        SaveResult& ioResult,
        GlyphConversionStats& ioGlyphStats,
        XpArtExporter::XpCell& outCell)
    {
        if (sourceCell.kind == CellKind::WideTrailing)
        {
            ioResult.hasEncodingFailure = true;
            ioResult.firstFailingCodePoint = sourceCell.glyph;
            ioResult.firstFailingPosition = { x, y };
            ioResult.errorMessage = "XP export cannot serialize WideTrailing cells directly. Flatten or normalize the TextObject before saving.";
            return false;
        }

        if (sourceCell.kind == CellKind::CombiningContinuation)
        {
            ioResult.hasEncodingFailure = true;
            ioResult.firstFailingCodePoint = sourceCell.glyph;
            ioResult.firstFailingPosition = { x, y };
            ioResult.errorMessage = "XP export cannot serialize CombiningContinuation cells directly. Flatten or normalize the TextObject before saving.";
            return false;
        }

        if (sourceCell.kind == CellKind::Glyph && sourceCell.width != CellWidth::One)
        {
            ioResult.hasEncodingFailure = true;
            ioResult.firstFailingCodePoint = sourceCell.glyph;
            ioResult.firstFailingPosition = { x, y };
            ioResult.errorMessage = "XP export currently requires every emitted glyph to occupy exactly one cell.";
            return false;
        }

        const CellStyleResolution styleInfo = resolveCellStyle(sourceCell.style);
        outCell.foreground = styleInfo.foreground;
        outCell.background = styleInfo.background;

        if (styleInfo.usedThemeColor)
        {
            addWarningOnce(
                ioResult,
                SaveWarningCode::XpThemeColorResolvedToRgb,
                "One or more authored theme colors were resolved to concrete RGB values for XP export.");
        }

        if (styleInfo.approximatedReverse)
        {
            addWarningOnce(
                ioResult,
                SaveWarningCode::XpReverseApproximated,
                "One or more reverse style flags were approximated by swapping foreground and background colors during XP export.");
        }

        if (styleInfo.approximatedInvisible)
        {
            addWarningOnce(
                ioResult,
                SaveWarningCode::XpInvisibleApproximated,
                "One or more invisible style flags were approximated by matching foreground to background during XP export.");
        }

        if (styleInfo.droppedUnsupportedStyle)
        {
            addWarningOnce(
                ioResult,
                SaveWarningCode::XpUnsupportedStyleDropped,
                "One or more TextObject style attributes are not representable in XP cell data and were dropped explicitly.");
        }

        return convertGlyphToXpCell(sourceCell, x, y, options, ioResult, ioGlyphStats, outCell);
    }

    void finalizeGlyphWarnings(SaveResult& ioResult, const GlyphConversionStats& stats)
    {
        if (stats.hasFallbackExample)
        {
            std::ostringstream message;
            message
                << stats.fallbackCount
                << " glyph(s) were approximated to nearby CP437 equivalents during XP export. First example: "
                << toHexCodePoint(stats.fallbackSourceCodePoint)
                << " -> "
                << toHexCodePoint(stats.fallbackReplacementCodePoint)
                << " at ("
                << stats.fallbackPosition.x
                << ", "
                << stats.fallbackPosition.y
                << ") because "
                << stats.fallbackReason
                << ".";

            addDetailedWarning(
                ioResult,
                SaveWarningCode::XpGlyphFallbackSubstituted,
                message.str(),
                stats.fallbackPosition,
                stats.fallbackSourceCodePoint,
                stats.fallbackReplacementCodePoint);
        }

        if (stats.hasReplacementExample)
        {
            std::ostringstream message;
            message
                << stats.replacementCount
                << " glyph(s) were replaced with the configured XP replacement glyph "
                << toHexCodePoint(stats.replacementGlyph)
                << ". First example: "
                << toHexCodePoint(stats.replacementSourceCodePoint)
                << " -> "
                << toHexCodePoint(stats.replacementGlyph)
                << " at ("
                << stats.replacementPosition.x
                << ", "
                << stats.replacementPosition.y
                << ").";

            addDetailedWarning(
                ioResult,
                SaveWarningCode::XpGlyphReplacementUsed,
                message.str(),
                stats.replacementPosition,
                stats.replacementSourceCodePoint,
                stats.replacementGlyph);
        }
    }
}

namespace XpArtExporter
{
    bool XpLayer::isValid() const
    {
        if (width <= 0 || height <= 0)
        {
            return false;
        }

        const std::uint64_t expected =
            static_cast<std::uint64_t>(width) *
            static_cast<std::uint64_t>(height);

        return cells.size() == static_cast<std::size_t>(expected);
    }

    bool XpDocument::isValid() const
    {
        if (canvasWidth <= 0 || canvasHeight <= 0 || layers.empty())
        {
            return false;
        }

        for (const XpLayer& layer : layers)
        {
            if (!layer.isValid())
            {
                return false;
            }
        }

        return true;
    }

    bool buildDocument(
        const TextObject& object,
        const TextObjectExporter::SaveOptions& options,
        TextObjectExporter::SaveResult& ioResult,
        XpDocument& outDocument)
    {
        outDocument = XpDocument{};
        outDocument.formatVersion = 1;
        outDocument.canvasWidth = object.getWidth();
        outDocument.canvasHeight = object.getHeight();

        if (!object.isLoaded())
        {
            ioResult.errorMessage = "Cannot export an unloaded TextObject to XP.";
            return false;
        }

        if (object.getWidth() <= 0 || object.getHeight() <= 0)
        {
            ioResult.errorMessage = "XP export requires a non-empty TextObject canvas.";
            return false;
        }

        bool countOk = false;
        const std::size_t cellCount = checkedCellCount(object.getWidth(), object.getHeight(), countOk);
        if (!countOk)
        {
            ioResult.errorMessage = "XP export cell count overflow.";
            return false;
        }

        XpLayer layer;
        layer.width = object.getWidth();
        layer.height = object.getHeight();
        layer.cells.resize(cellCount);

        GlyphConversionStats glyphStats;

        for (int y = 0; y < object.getHeight(); ++y)
        {
            for (int x = 0; x < object.getWidth(); ++x)
            {
                const TextObjectCell& sourceCell = object.getCell(x, y);
                XpCell convertedCell;
                if (!convertTextObjectCell(sourceCell, x, y, options, ioResult, glyphStats, convertedCell))
                {
                    return false;
                }

                const std::size_t index = static_cast<std::size_t>(layerIndex(x, y, layer.height));
                layer.cells[index] = convertedCell;
            }
        }

        finalizeGlyphWarnings(ioResult, glyphStats);

        outDocument.layers.clear();
        outDocument.layers.push_back(std::move(layer));
        return outDocument.isValid();
    }

    bool serializeDocument(
        const XpDocument& document,
        TextObjectExporter::SaveResult& ioResult,
        std::string& outBytes)
    {
        outBytes.clear();

        if (!document.isValid())
        {
            ioResult.errorMessage = "XP export document is invalid before serialization.";
            return false;
        }

        std::string payload;
        payload.reserve(static_cast<std::size_t>(8 + document.layers.size() * 8));

        const std::int32_t versionHeader = -document.formatVersion;
        appendLe32(payload, static_cast<std::uint32_t>(versionHeader));
        appendLe32(payload, static_cast<std::uint32_t>(document.layers.size()));

        for (const XpLayer& layer : document.layers)
        {
            appendLe32(payload, static_cast<std::uint32_t>(layer.width));
            appendLe32(payload, static_cast<std::uint32_t>(layer.height));

            for (const XpCell& cell : layer.cells)
            {
                appendLe32(payload, cell.glyph);
                payload.push_back(static_cast<char>(cell.foreground.red));
                payload.push_back(static_cast<char>(cell.foreground.green));
                payload.push_back(static_cast<char>(cell.foreground.blue));
                payload.push_back(static_cast<char>(cell.background.red));
                payload.push_back(static_cast<char>(cell.background.green));
                payload.push_back(static_cast<char>(cell.background.blue));
            }
        }

        return gzipStoreOnly(payload, outBytes);
    }

    bool exportToBytes(
        const TextObject& object,
        const TextObjectExporter::SaveOptions& options,
        TextObjectExporter::SaveResult& ioResult)
    {
        XpDocument document;
        if (!buildDocument(object, options, ioResult, document))
        {
            return false;
        }

        if (!serializeDocument(document, ioResult, ioResult.bytes))
        {
            return false;
        }

        ioResult.success = true;
        return true;
    }
}

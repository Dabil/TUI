#include "Rendering/Objects/XpArtExporter.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

#include "Rendering/Capabilities/ColorSupport.h"
#include "Rendering/Styles/ColorMapping.h"
#include "Rendering/Styles/ColorResolver.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/ThemeColor.h"

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

    bool tryEncodeCp437Byte(char32_t codePoint, unsigned char& outByte)
    {
        if (codePoint <= 0x7Fu)
        {
            outByte = static_cast<unsigned char>(codePoint);
            return true;
        }

        static const std::array<char32_t, 128> kCp437Table =
        {
            U'\u00C7', U'\u00FC', U'\u00E9', U'\u00E2', U'\u00E4', U'\u00E0', U'\u00E5', U'\u00E7',
            U'\u00EA', U'\u00EB', U'\u00E8', U'\u00EF', U'\u00EE', U'\u00EC', U'\u00C4', U'\u00C5',
            U'\u00C9', U'\u00E6', U'\u00C6', U'\u00F4', U'\u00F6', U'\u00F2', U'\u00FB', U'\u00F9',
            U'\u00FF', U'\u00D6', U'\u00DC', U'\u00A2', U'\u00A3', U'\u00A5', U'\u20A7', U'\u0192',
            U'\u00E1', U'\u00ED', U'\u00F3', U'\u00FA', U'\u00F1', U'\u00D1', U'\u00AA', U'\u00BA',
            U'\u00BF', U'\u2310', U'\u00AC', U'\u00BD', U'\u00BC', U'\u00A1', U'\u00AB', U'\u00BB',
            U'\u2591', U'\u2592', U'\u2593', U'\u2502', U'\u2524', U'\u2561', U'\u2562', U'\u2556',
            U'\u2555', U'\u2563', U'\u2551', U'\u2557', U'\u255D', U'\u255C', U'\u255B', U'\u2510',
            U'\u2514', U'\u2534', U'\u252C', U'\u251C', U'\u2500', U'\u253C', U'\u255E', U'\u255F',
            U'\u255A', U'\u2554', U'\u2569', U'\u2566', U'\u2560', U'\u2550', U'\u256C', U'\u2567',
            U'\u2568', U'\u2564', U'\u2565', U'\u2559', U'\u2558', U'\u2552', U'\u2553', U'\u256B',
            U'\u256A', U'\u2518', U'\u250C', U'\u2588', U'\u2584', U'\u258C', U'\u2590', U'\u2580',
            U'\u03B1', U'\u00DF', U'\u0393', U'\u03C0', U'\u03A3', U'\u03C3', U'\u00B5', U'\u03C4',
            U'\u03A6', U'\u0398', U'\u03A9', U'\u03B4', U'\u221E', U'\u03C6', U'\u03B5', U'\u2229',
            U'\u2261', U'\u00B1', U'\u2265', U'\u2264', U'\u2320', U'\u2321', U'\u00F7', U'\u2248',
            U'\u00B0', U'\u2219', U'\u00B7', U'\u221A', U'\u207F', U'\u00B2', U'\u25A0', U'\u00A0'
        };

        for (std::size_t index = 0; index < kCp437Table.size(); ++index)
        {
            if (kCp437Table[index] == codePoint)
            {
                outByte = static_cast<unsigned char>(0x80u + index);
                return true;
            }
        }

        return false;
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

    bool convertTextObjectCell(
        const TextObjectCell& sourceCell,
        int x,
        int y,
        SaveResult& ioResult,
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

        char32_t glyph = U' ';
        if (sourceCell.kind == CellKind::Glyph)
        {
            glyph = sourceCell.glyph;
        }

        unsigned char cp437Byte = 0;
        if (tryEncodeCp437Byte(glyph, cp437Byte))
        {
            outCell.glyph = static_cast<std::uint32_t>(cp437Byte);
            return true;
        }

        outCell.glyph = static_cast<std::uint32_t>(glyph);
        addWarningOnce(
            ioResult,
            SaveWarningCode::XpUnicodeGlyphStoredDirectly,
            "One or more glyphs are not representable as CP437 and were stored directly as Unicode scalar values in XP cell data.");
        return true;
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
        (void)options;

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

        for (int y = 0; y < object.getHeight(); ++y)
        {
            for (int x = 0; x < object.getWidth(); ++x)
            {
                const TextObjectCell& sourceCell = object.getCell(x, y);
                XpCell convertedCell;
                if (!convertTextObjectCell(sourceCell, x, y, ioResult, convertedCell))
                {
                    return false;
                }

                const std::size_t index = static_cast<std::size_t>(layerIndex(x, y, layer.height));
                layer.cells[index] = convertedCell;
            }
        }

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

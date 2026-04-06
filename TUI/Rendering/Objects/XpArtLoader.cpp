#include "Rendering/Objects/XpArtLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Rendering/Objects/TextObjectBuilder.h"
#include "Rendering/Styles/Color.h"
#include "Utilities/Unicode/UnicodeConversion.h"

#if defined(__has_include)
#   if __has_include(<zlib.h>)
#       include <zlib.h>
#       define TUI_XP_ART_LOADER_HAS_ZLIB 1
#   else
#       define TUI_XP_ART_LOADER_HAS_ZLIB 0
#   endif
#else
#   define TUI_XP_ART_LOADER_HAS_ZLIB 0
#endif

namespace
{
    constexpr std::size_t kVersionBytes = 4;
    constexpr std::size_t kLayerCountBytes = 4;
    constexpr std::size_t kLayerWidthBytes = 4;
    constexpr std::size_t kLayerHeightBytes = 4;
    constexpr std::size_t kGlyphBytes = 4;
    constexpr std::size_t kForegroundRgbBytes = 3;
    constexpr std::size_t kBackgroundRgbBytes = 3;
    constexpr std::size_t kCellBytes =
        kGlyphBytes +
        kForegroundRgbBytes +
        kBackgroundRgbBytes;

    const XpArtLoader::RgbColor kTransparentBackground{ 255, 0, 255 };
    const XpArtLoader::RgbColor kOpaqueBlack{ 0, 0, 0 };

    std::string toLowerCopy(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

        return value;
    }

    std::string getFileExtensionLower(const std::string& filePath)
    {
        const std::size_t dot = filePath.find_last_of('.');
        if (dot == std::string::npos)
        {
            return {};
        }

        return toLowerCopy(filePath.substr(dot));
    }

    bool readAllBytes(const std::string& filePath, std::string& outBytes)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file)
        {
            return false;
        }

        outBytes.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());

        return true;
    }

    bool looksLikeGzip(std::string_view bytes)
    {
        return bytes.size() >= 2 &&
            static_cast<unsigned char>(bytes[0]) == 0x1F &&
            static_cast<unsigned char>(bytes[1]) == 0x8B;
    }

    std::uint32_t readLe32(std::string_view bytes, std::size_t offset, bool& ok)
    {
        if (offset + 3 >= bytes.size())
        {
            ok = false;
            return 0;
        }

        ok = true;
        return
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 0])) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 8) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 16) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3])) << 24);
    }

    char32_t decodeCp437Byte(unsigned char value)
    {
        if (value <= 0x7F)
        {
            return UnicodeConversion::sanitizeCodePoint(static_cast<char32_t>(value));
        }

        static const char32_t kCp437Table[128] =
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

        return UnicodeConversion::sanitizeCodePoint(kCp437Table[value - 0x80]);
    }

    char32_t decodeRexPaintGlyph(std::uint32_t glyph)
    {
        if (glyph <= 0xFFu)
        {
            return decodeCp437Byte(static_cast<unsigned char>(glyph));
        }

        return UnicodeConversion::sanitizeCodePoint(static_cast<char32_t>(glyph));
    }

    int layerIndex(int x, int y, int height)
    {
        return (x * height) + y;
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

    void addWarning(
        XpArtLoader::LoadResult& result,
        XpArtLoader::LoadWarningCode code,
        const std::string& message,
        std::size_t byteOffset = 0,
        const XpArtLoader::SourcePosition& position = {})
    {
        XpArtLoader::LoadWarning warning;
        warning.code = code;
        warning.message = message;
        warning.byteOffset = byteOffset;
        warning.sourcePosition = position;
        result.warnings.push_back(warning);
    }

    XpArtLoader::ParseResult failParse(
        const std::string& message,
        std::size_t byteOffset = 0,
        const XpArtLoader::SourcePosition& position = {})
    {
        XpArtLoader::ParseResult result;
        result.success = false;
        result.errorMessage = message;
        result.hasParseFailure = true;
        result.failingByteOffset = byteOffset;
        result.firstFailingPosition = position;
        return result;
    }

    XpArtLoader::LoadResult failLoad(
        XpArtLoader::FileType fileType,
        const std::string& message,
        std::size_t byteOffset = 0,
        const XpArtLoader::SourcePosition& position = {})
    {
        XpArtLoader::LoadResult result;
        result.success = false;
        result.detectedFileType = fileType;
        result.errorMessage = message;
        result.hasParseFailure = true;
        result.failingByteOffset = byteOffset;
        result.firstFailingPosition = position;
        return result;
    }

#if TUI_XP_ART_LOADER_HAS_ZLIB
    bool tryInflateXpBytes(std::string_view bytes, std::string& outDecompressed)
    {
        outDecompressed.clear();

        z_stream stream{};
        stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(bytes.data()));
        stream.avail_in = static_cast<uInt>(bytes.size());

        if (inflateInit2(&stream, 15 + 32) != Z_OK)
        {
            return false;
        }

        constexpr std::size_t kChunkSize = 16384;
        std::vector<char> chunk(kChunkSize);

        int rc = Z_OK;
        while (rc == Z_OK)
        {
            stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
            stream.avail_out = static_cast<uInt>(chunk.size());

            rc = inflate(&stream, Z_NO_FLUSH);
            if (rc != Z_OK && rc != Z_STREAM_END)
            {
                inflateEnd(&stream);
                outDecompressed.clear();
                return false;
            }

            const std::size_t produced = chunk.size() - static_cast<std::size_t>(stream.avail_out);
            outDecompressed.append(chunk.data(), produced);
        }

        inflateEnd(&stream);
        return rc == Z_STREAM_END;
    }
#endif

    XpArtLoader::ParseResult parseXpPayloadInternal(std::string_view bytes)
    {
        using namespace XpArtLoader;

        if (bytes.size() < (kVersionBytes + kLayerCountBytes))
        {
            return failParse("XP payload is too small to contain a header.");
        }

        ParseResult result;
        result.success = false;

        std::size_t offset = 0;
        bool ok = false;

        const std::int32_t rawVersion = static_cast<std::int32_t>(readLe32(bytes, offset, ok));
        if (!ok)
        {
            return failParse("Failed to read XP version header.", offset);
        }
        offset += kVersionBytes;

        std::uint32_t layerCount = 0;

        if (rawVersion < 0)
        {
            result.document.formatVersion = -rawVersion;
            layerCount = readLe32(bytes, offset, ok);
            if (!ok)
            {
                return failParse("Failed to read XP layer count.", offset);
            }
            offset += kLayerCountBytes;
        }
        else
        {
            // Legacy header: version actually contains layer count
            result.document.usesLegacyLayerCountHeader = true;
            result.document.formatVersion = 0;
            layerCount = static_cast<std::uint32_t>(rawVersion);
        }

        if (layerCount == 0)
        {
            return failParse("XP file contains zero layers.", offset);
        }

        result.document.layers.reserve(layerCount);

        int maxWidth = 0;
        int maxHeight = 0;

        for (std::uint32_t li = 0; li < layerCount; ++li)
        {
            bool okLayer = false;

            const int width = static_cast<int>(readLe32(bytes, offset, okLayer));
            if (!okLayer)
                return failParse("Failed to read layer width.", offset);
            offset += kLayerWidthBytes;

            const int height = static_cast<int>(readLe32(bytes, offset, okLayer));
            if (!okLayer)
                return failParse("Failed to read layer height.", offset);
            offset += kLayerHeightBytes;

            if (width <= 0 || height <= 0)
                return failParse("Invalid layer dimensions.", offset);

            bool countOk = false;
            const std::size_t cellCount = checkedCellCount(width, height, countOk);
            if (!countOk)
                return failParse("Layer cell count overflow.", offset);

            LayerData layer;
            layer.width = width;
            layer.height = height;
            layer.cells.resize(cellCount);

            for (std::size_t i = 0; i < cellCount; ++i)
            {
                if (offset + kCellBytes > bytes.size())
                    return failParse("Unexpected end of XP cell data.", offset);

                bool glyphOk = false;
                const std::uint32_t glyph = readLe32(bytes, offset, glyphOk);
                if (!glyphOk)
                    return failParse("Failed to read glyph.", offset);
                offset += kGlyphBytes;

                CellData& cell = layer.cells[i];
                cell.glyph = glyph;

                cell.foreground.red = static_cast<uint8_t>(bytes[offset++]);
                cell.foreground.green = static_cast<uint8_t>(bytes[offset++]);
                cell.foreground.blue = static_cast<uint8_t>(bytes[offset++]);

                cell.background.red = static_cast<uint8_t>(bytes[offset++]);
                cell.background.green = static_cast<uint8_t>(bytes[offset++]);
                cell.background.blue = static_cast<uint8_t>(bytes[offset++]);
            }

            maxWidth = std::max(maxWidth, width);
            maxHeight = std::max(maxHeight, height);

            result.document.layers.push_back(std::move(layer));
        }

        result.document.canvasWidth = maxWidth;
        result.document.canvasHeight = maxHeight;

        result.bytesConsumed = offset;
        result.success = true;
        return result;
    }

    Color toColor(const XpArtLoader::RgbColor& rgb)
    {
        return Color::FromRgb(rgb.red, rgb.green, rgb.blue);
    }
}

// ============================================================
// PUBLIC API IMPLEMENTATION
// ============================================================

namespace XpArtLoader
{
    ParseResult parseDecompressedPayload(std::string_view bytes)
    {
        return parseXpPayloadInternal(bytes);
    }

    LoadResult buildTextObject(const ParsedDocument& document, const LoadOptions& options)
    {
        LoadResult result;

        if (!document.isValid())
        {
            result.success = false;
            result.errorMessage = "Invalid parsed XP document.";
            return result;
        }

        TextObjectBuilder builder(document.canvasWidth, document.canvasHeight);

        if (document.layers.empty())
        {
            result.success = true;
            result.object = builder.build();
            return result;
        }

        const bool flatten = options.flattenLayers;

        if (flatten && document.layers.size() > 1)
        {
            result.usedLayerFlattening = true;
            addWarning(result,
                LoadWarningCode::MultipleLayersFlattened,
                "Multiple layers flattened into single TextObject.");
        }

        const int layerCount = static_cast<int>(document.layers.size());

        for (int li = 0; li < layerCount; ++li)
        {
            const auto& layer = document.layers[li];

            for (int x = 0; x < layer.width; ++x)
            {
                for (int y = 0; y < layer.height; ++y)
                {
                    const CellData* cell = layer.tryGetCell(x, y);
                    if (!cell) continue;

                    const bool isTransparent =
                        options.treatMagentaBackgroundAsTransparent &&
                        cell->background == kTransparentBackground;

                    if (isTransparent)
                    {
                        addWarning(result,
                            LoadWarningCode::TransparentCellsSkipped,
                            "Transparent XP cell skipped.",
                            0,
                            { x, y });
                        continue;
                    }

                    const char32_t ch = decodeRexPaintGlyph(cell->glyph);

                    Style style = options.baseStyle.value_or(Style{});
                    style = style.withForeground(toColor(cell->foreground));

                    if (!isTransparent)
                    {
                        style = style.withBackground(toColor(cell->background));
                    }
                    else if (options.visibleTransparentBaseCellsUseBlackBackground)
                    {
                        style = style.withBackground(toColor(kOpaqueBlack));
                    }

                    builder.setGlyph(x, y, ch, style);
                }
            }

            if (!flatten)
                break;
        }

        result.object = builder.build();
        result.success = true;
        result.resolvedWidth = document.canvasWidth;
        result.resolvedHeight = document.canvasHeight;
        result.resolvedLayerCount = static_cast<int>(document.layers.size());
        result.parsedFormatVersion = document.formatVersion;

        return result;
    }

    LoadResult loadFromBytes(std::string_view bytes, const LoadOptions& options)
    {
        LoadResult result;
        result.detectedFileType = FileType::Xp;

        std::string decompressed;

#if TUI_XP_ART_LOADER_HAS_ZLIB
        if (options.allowCompressedInput && looksLikeGzip(bytes))
        {
            if (!tryInflateXpBytes(bytes, decompressed))
                return failLoad(FileType::Xp, "Failed to decompress XP data.");

            result.inputWasCompressed = true;
            bytes = decompressed;
        }
        else if (looksLikeGzip(bytes))
        {
            return failLoad(FileType::Xp, "Compressed XP input not allowed.");
        }
#else
        if (looksLikeGzip(bytes))
        {
            return failLoad(FileType::Xp, "XP file appears compressed but zlib is unavailable.");
        }
#endif

        ParseResult parse = parseDecompressedPayload(bytes);

        if (!parse.success)
        {
            return failLoad(FileType::Xp,
                parse.errorMessage,
                parse.failingByteOffset,
                parse.firstFailingPosition);
        }

        result.parsedFormatVersion = parse.document.formatVersion;
        result.inputWasAlreadyDecompressed = parse.inputWasAlreadyDecompressed;

        return buildTextObject(parse.document, options);
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject)
    {
        LoadResult r = loadFromFile(filePath);
        if (!r.success) return false;
        outObject = r.object;
        return true;
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const LoadOptions& options)
    {
        LoadResult r = loadFromFile(filePath, options);
        if (!r.success) return false;
        outObject = r.object;
        return true;
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const Style& style)
    {
        LoadResult r = loadFromFile(filePath, style);
        if (!r.success) return false;
        outObject = r.object;
        return true;
    }

    bool CellData::hasTransparentBackground() const
    {
        return background == RgbColor{ 255, 0, 255 };
    }

    bool LayerData::isValid() const
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

    bool LayerData::inBounds(int x, int y) const
    {
        return x >= 0 && y >= 0 && x < width && y < height;
    }

    const CellData* LayerData::tryGetCell(int x, int y) const
    {
        if (!inBounds(x, y))
        {
            return nullptr;
        }

        const std::size_t index =
            static_cast<std::size_t>((x * height) + y);

        if (index >= cells.size())
        {
            return nullptr;
        }

        return &cells[index];
    }

    bool ParsedDocument::isValid() const
    {
        if (layers.empty())
        {
            return false;
        }

        if (canvasWidth <= 0 || canvasHeight <= 0)
        {
            return false;
        }

        for (const LayerData& layer : layers)
        {
            if (!layer.isValid())
            {
                return false;
            }
        }

        return true;
    }

    LoadResult loadFromFile(const std::string& filePath)
    {
        return loadFromFile(filePath, LoadOptions{});
    }

    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options)
    {
        std::string bytes;
        if (!readAllBytes(filePath, bytes))
        {
            return failLoad(FileType::Xp, "Failed to read file: " + filePath);
        }

        LoadResult result = loadFromBytes(bytes, options);
        if (result.detectedFileType == FileType::Unknown)
        {
            result.detectedFileType = detectFileType(filePath);
        }
        return result;
    }

    LoadResult loadFromFile(const std::string& filePath, const Style& style)
    {
        LoadOptions options;
        options.baseStyle = style;
        return loadFromFile(filePath, options);
    }

    FileType detectFileType(const std::string& filePath)
    {
        const std::string extension = getFileExtensionLower(filePath);

        if (extension == ".xp")
        {
            return FileType::Xp;
        }

        return FileType::Unknown;
    }
}
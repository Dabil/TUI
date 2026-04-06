#include "Rendering/Objects/BinaryArtLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "Rendering/Styles/Color.h"
#include "Utilities/Unicode/UnicodeConversion.h"

namespace
{
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

    Color::Basic dosBasicColor(int value)
    {
        switch (value & 0x0F)
        {
        case 0x0: return Color::Basic::Black;
        case 0x1: return Color::Basic::Blue;
        case 0x2: return Color::Basic::Green;
        case 0x3: return Color::Basic::Cyan;
        case 0x4: return Color::Basic::Red;
        case 0x5: return Color::Basic::Magenta;
        case 0x6: return Color::Basic::Yellow;
        case 0x7: return Color::Basic::White;
        case 0x8: return Color::Basic::BrightBlack;
        case 0x9: return Color::Basic::BrightBlue;
        case 0xA: return Color::Basic::BrightGreen;
        case 0xB: return Color::Basic::BrightCyan;
        case 0xC: return Color::Basic::BrightRed;
        case 0xD: return Color::Basic::BrightMagenta;
        case 0xE: return Color::Basic::BrightYellow;
        case 0xF: return Color::Basic::BrightWhite;
        default: return Color::Basic::White;
        }
    }

    std::uint16_t readLe16(std::string_view bytes, std::size_t offset, bool& ok)
    {
        if (offset + 1 >= bytes.size())
        {
            ok = false;
            return 0;
        }

        ok = true;

        return static_cast<std::uint16_t>(
            static_cast<unsigned char>(bytes[offset]) |
            (static_cast<unsigned char>(bytes[offset + 1]) << 8));
    }

    Style mergeBaseStyle(const std::optional<Style>& baseStyle, unsigned char attribute, bool iceColors)
    {
        Style style = baseStyle.value_or(Style{});

        const int foreground = attribute & 0x0F;
        const int backgroundNibble = (attribute >> 4) & 0x0F;

        style = style.withForeground(Color::FromBasic(dosBasicColor(foreground)));

        if (iceColors)
        {
            style = style.withBackground(Color::FromBasic(dosBasicColor(backgroundNibble)));
        }
        else
        {
            const int background = (attribute >> 4) & 0x07;
            style = style.withBackground(Color::FromBasic(dosBasicColor(background)));

            if ((attribute & 0x80) != 0)
            {
                style = style.withSlowBlink(true);
            }
        }

        return style;
    }

    TextObject makeBlankObject(int width, int height)
    {
        width = std::max(1, width);
        height = std::max(1, height);

        std::u32string blank;
        blank.reserve(static_cast<std::size_t>(width * height + height));

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                blank.push_back(U' ');
            }

            if (y + 1 < height)
            {
                blank.push_back(U'\n');
            }
        }

        TextObject object = TextObject::fromU32(blank);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                TextObjectCell& cell = const_cast<TextObjectCell&>(object.getCell(x, y));
                cell.glyph = U' ';
                cell.kind = CellKind::Glyph;
                cell.width = CellWidth::One;
                cell.style.reset();
            }
        }

        return object;
    }

    BinaryArtLoader::LoadResult decodeBinLike(
        std::string_view cellBytes,
        int width,
        const BinaryArtLoader::LoadOptions& options,
        BinaryArtLoader::FileType detectedType,
        bool usedIceColors)
    {
        BinaryArtLoader::LoadResult result;
        result.detectedFileType = detectedType;
        result.usedIceColors = usedIceColors;

        if (width <= 0)
        {
            result.success = false;
            result.errorMessage = "Binary art width must be greater than zero.";
            return result;
        }

        if ((cellBytes.size() % 2) != 0)
        {
            result.success = false;
            result.errorMessage = "Binary art byte count must be an even number.";
            return result;
        }

        const std::size_t cellCount = cellBytes.size() / 2;
        if ((cellCount % static_cast<std::size_t>(width)) != 0)
        {
            result.success = false;
            result.errorMessage = "Binary art cell count does not align to the configured width.";
            return result;
        }

        const int height = static_cast<int>(cellCount / static_cast<std::size_t>(width));

        TextObject object = makeBlankObject(width, height);

        std::size_t offset = 0;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const unsigned char glyphByte = static_cast<unsigned char>(cellBytes[offset++]);
                const unsigned char attrByte = static_cast<unsigned char>(cellBytes[offset++]);

                TextObjectCell& cell = const_cast<TextObjectCell&>(object.getCell(x, y));
                cell.glyph = decodeCp437Byte(glyphByte);
                cell.kind = CellKind::Glyph;
                cell.width = CellWidth::One;
                cell.style = mergeBaseStyle(options.baseStyle, attrByte, usedIceColors);
            }
        }

        result.object = object;
        result.success = true;
        result.resolvedWidth = width;
        result.resolvedHeight = height;
        return result;
    }

    BinaryArtLoader::LoadResult loadBin(
        std::string_view bytes,
        const BinaryArtLoader::LoadOptions& options)
    {
        return decodeBinLike(
            bytes,
            std::max(1, options.defaultColumns),
            options,
            BinaryArtLoader::FileType::Bin,
            options.enableIceColors);
    }

    BinaryArtLoader::LoadResult loadAdf(
        std::string_view bytes,
        const BinaryArtLoader::LoadOptions& options)
    {
        BinaryArtLoader::LoadResult result = decodeBinLike(
            bytes,
            std::max(1, options.defaultColumns),
            options,
            BinaryArtLoader::FileType::Adf,
            options.enableIceColors);

        if (result.success)
        {
            result.warnings.push_back({
                BinaryArtLoader::LoadWarningCode::AdfSubsetAssumed,
                "ADF was loaded using the initial bin-like ArtWorx subset. Custom palette/font semantics are not yet interpreted."
            });
        }

        return result;
    }

    BinaryArtLoader::LoadResult loadXBin(
        std::string_view bytes,
        const BinaryArtLoader::LoadOptions& options)
    {
        BinaryArtLoader::LoadResult result;
        result.detectedFileType = BinaryArtLoader::FileType::XBin;

        if (bytes.size() < 11)
        {
            result.success = false;
            result.errorMessage = "XBIN file is too small to contain a valid header.";
            return result;
        }

        if (!(bytes[0] == 'X' && bytes[1] == 'B' && bytes[2] == 'I' && bytes[3] == 'N' &&
              static_cast<unsigned char>(bytes[4]) == 0x1A))
        {
            result.success = false;
            result.errorMessage = "Missing XBIN signature.";
            return result;
        }

        bool ok = false;
        const std::uint16_t width = readLe16(bytes, 5, ok);
        if (!ok)
        {
            result.success = false;
            result.errorMessage = "Failed to read XBIN width.";
            return result;
        }

        const std::uint16_t height = readLe16(bytes, 7, ok);
        if (!ok)
        {
            result.success = false;
            result.errorMessage = "Failed to read XBIN height.";
            return result;
        }

        const unsigned char fontHeight = static_cast<unsigned char>(bytes[9]);
        const unsigned char flags = static_cast<unsigned char>(bytes[10]);

        const bool hasPalette = (flags & 0x01) != 0;
        const bool hasFont = (flags & 0x02) != 0;
        const bool isCompressed = (flags & 0x04) != 0;
        const bool nonBlinkMode = (flags & 0x08) != 0;
        const bool has512Chars = (flags & 0x10) != 0;

        if (width == 0 || height == 0)
        {
            result.success = false;
            result.errorMessage = "XBIN width and height must be non-zero.";
            return result;
        }

        if (isCompressed)
        {
            result.success = false;
            result.errorMessage = "Compressed XBIN is not yet supported.";
            result.warnings.push_back({
                BinaryArtLoader::LoadWarningCode::XBinCompressionUnsupported,
                "Compressed XBIN data was detected and rejected explicitly."
            });
            return result;
        }

        std::size_t offset = 11;

        if (hasPalette)
        {
            if (offset + 48 > bytes.size())
            {
                result.success = false;
                result.errorMessage = "XBIN palette section is truncated.";
                return result;
            }

            offset += 48;
            result.warnings.push_back({
                BinaryArtLoader::LoadWarningCode::PaletteDataIgnored,
                "XBIN palette data was present but is not yet mapped into engine theme/palette state."
            });
        }

        if (hasFont)
        {
            const std::size_t glyphCount = has512Chars ? 512u : 256u;
            const std::size_t fontBytes = glyphCount * static_cast<std::size_t>(fontHeight);

            if (offset + fontBytes > bytes.size())
            {
                result.success = false;
                result.errorMessage = "XBIN font section is truncated.";
                return result;
            }

            offset += fontBytes;
            result.warnings.push_back({
                BinaryArtLoader::LoadWarningCode::FontDataIgnored,
                "XBIN font data was present but is not yet imported into engine font/glyph policy."
            });
        }

        const std::size_t expectedCellBytes =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height) * 2u;

        if (offset + expectedCellBytes > bytes.size())
        {
            result.success = false;
            result.errorMessage = "XBIN character/attribute data is truncated.";
            return result;
        }

        const bool iceColors = nonBlinkMode || options.enableIceColors;
        BinaryArtLoader::LoadResult decoded = decodeBinLike(
            bytes.substr(offset, expectedCellBytes),
            static_cast<int>(width),
            options,
            BinaryArtLoader::FileType::XBin,
            iceColors);

        decoded.usedIceColors = iceColors;
        decoded.warnings.insert(
            decoded.warnings.begin(),
            result.warnings.begin(),
            result.warnings.end());

        if (offset + expectedCellBytes < bytes.size())
        {
            decoded.warnings.push_back({
                BinaryArtLoader::LoadWarningCode::ExtraTrailingBytesIgnored,
                "Extra trailing bytes after XBIN payload were ignored."
            });
        }

        return decoded;
    }
}

namespace BinaryArtLoader
{
    FileType detectFileType(const std::string& filePath)
    {
        const std::string extension = getFileExtensionLower(filePath);

        if (extension == ".bin")
        {
            return FileType::Bin;
        }

        if (extension == ".xbin" || extension == ".xb")
        {
            return FileType::XBin;
        }

        if (extension == ".adf")
        {
            return FileType::Adf;
        }

        return FileType::Unknown;
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
            LoadResult result;
            result.success = false;
            result.detectedFileType = detectFileType(filePath);
            result.errorMessage = "Failed to open binary art file.";
            return result;
        }

        LoadResult result = loadFromBytes(bytes, options);
        result.detectedFileType = detectFileType(filePath);
        return result;
    }

    LoadResult loadFromFile(const std::string& filePath, const Style& style)
    {
        LoadOptions options;
        options.baseStyle = style;
        return loadFromFile(filePath, options);
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject)
    {
        const LoadResult result = loadFromFile(filePath);
        if (!result.success)
        {
            return false;
        }

        outObject = result.object;
        return true;
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const LoadOptions& options)
    {
        const LoadResult result = loadFromFile(filePath, options);
        if (!result.success)
        {
            return false;
        }

        outObject = result.object;
        return true;
    }

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const Style& style)
    {
        const LoadResult result = loadFromFile(filePath, style);
        if (!result.success)
        {
            return false;
        }

        outObject = result.object;
        return true;
    }

    LoadResult loadFromBytes(std::string_view bytes, const LoadOptions& options)
    {
        const FileType type = options.fileType;

        if (type == FileType::XBin ||
            (bytes.size() >= 5 &&
             bytes[0] == 'X' && bytes[1] == 'B' && bytes[2] == 'I' && bytes[3] == 'N' &&
             static_cast<unsigned char>(bytes[4]) == 0x1A))
        {
            return loadXBin(bytes, options);
        }

        if (type == FileType::Adf)
        {
            return loadAdf(bytes, options);
        }

        if (type == FileType::Bin || type == FileType::Auto || type == FileType::Unknown)
        {
            return loadBin(bytes, options);
        }

        LoadResult result;
        result.success = false;
        result.detectedFileType = FileType::Unknown;
        result.errorMessage = "Unsupported binary art file type.";
        return result;
    }

    const char* toString(FileType fileType)
    {
        switch (fileType)
        {
        case FileType::Auto: return "Auto";
        case FileType::Unknown: return "Unknown";
        case FileType::Bin: return "Bin";
        case FileType::XBin: return "XBin";
        case FileType::Adf: return "Adf";
        default: return "Unknown";
        }
    }

    const char* toString(LoadWarningCode warningCode)
    {
        switch (warningCode)
        {
        case LoadWarningCode::None: return "None";
        case LoadWarningCode::PaletteDataIgnored: return "PaletteDataIgnored";
        case LoadWarningCode::FontDataIgnored: return "FontDataIgnored";
        case LoadWarningCode::AdfSubsetAssumed: return "AdfSubsetAssumed";
        case LoadWarningCode::XBinCompressionUnsupported: return "XBinCompressionUnsupported";
        case LoadWarningCode::ExtraTrailingBytesIgnored: return "ExtraTrailingBytesIgnored";
        default: return "Unknown";
        }
    }
}
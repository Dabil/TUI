#include "Rendering/Objects/BinaryArtLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "Rendering/Objects/TextObjectBuilder.h"
#include "Rendering/Styles/Color.h"
#include "Utilities/Unicode/UnicodeConversion.h"

namespace
{
    struct SauceRecord
    {
        bool found = false;
        bool truncated = false;
        std::size_t contentSize = 0;
        BinaryArtLoader::SauceMetadata metadata;
    };

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

    std::string trimRightAscii(const std::string& value)
    {
        std::size_t end = value.size();
        while (end > 0 && (value[end - 1] == ' ' || value[end - 1] == '\0'))
        {
            --end;
        }

        return value.substr(0, end);
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

    SauceRecord parseSauce(std::string_view bytes)
    {
        SauceRecord result;
        result.contentSize = bytes.size();

        if (bytes.size() < 128)
        {
            return result;
        }

        const std::size_t sauceOffset = bytes.size() - 128;
        const std::string_view sauce = bytes.substr(sauceOffset, 128);

        if (sauce.substr(0, 5) != "SAUCE")
        {
            return result;
        }

        result.found = true;
        result.contentSize = sauceOffset;

        result.metadata.present = true;
        result.metadata.title = trimRightAscii(std::string(sauce.substr(7, 35)));
        result.metadata.author = trimRightAscii(std::string(sauce.substr(42, 20)));
        result.metadata.group = trimRightAscii(std::string(sauce.substr(62, 20)));
        result.metadata.date = trimRightAscii(std::string(sauce.substr(82, 8)));
        result.metadata.dataType = static_cast<std::uint8_t>(sauce[94]);
        result.metadata.fileType = static_cast<std::uint8_t>(sauce[95]);

        bool ok = false;
        result.metadata.tInfo1 = readLe16(sauce, 96, ok);
        result.metadata.tInfo2 = readLe16(sauce, 98, ok);
        result.metadata.tInfo3 = readLe16(sauce, 100, ok);
        result.metadata.tInfo4 = readLe16(sauce, 102, ok);
        result.metadata.commentLineCount = static_cast<std::uint8_t>(sauce[104]);
        result.metadata.flags = static_cast<std::uint8_t>(sauce[105]);
        result.metadata.fontName = trimRightAscii(std::string(sauce.substr(106, 22)));

        const std::size_t commentBytes =
            (result.metadata.commentLineCount > 0)
            ? (5u + static_cast<std::size_t>(result.metadata.commentLineCount) * 64u)
            : 0u;

        if (commentBytes > 0)
        {
            if (result.contentSize < commentBytes)
            {
                result.truncated = true;
                return result;
            }

            const std::size_t commentBlockOffset = result.contentSize - commentBytes;
            if (bytes.substr(commentBlockOffset, 5) == "COMNT")
            {
                result.contentSize = commentBlockOffset;
            }
        }

        return result;
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

    Style styleFromDosAttribute(
        unsigned char attribute,
        const std::optional<Style>& baseStyle,
        bool iceColors)
    {
        Style style = baseStyle.value_or(Style{});

        const int foreground = attribute & 0x0F;
        style = style.withForeground(Color::FromBasic(dosBasicColor(foreground)));

        if (iceColors)
        {
            const int background = (attribute >> 4) & 0x0F;
            style = style.withBackground(Color::FromBasic(dosBasicColor(background)));
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

    void addWarning(
        BinaryArtLoader::LoadResult& result,
        BinaryArtLoader::LoadWarningCode code,
        const std::string& message,
        std::size_t byteOffset,
        const BinaryArtLoader::SourcePosition& position)
    {
        BinaryArtLoader::LoadWarning warning;
        warning.code = code;
        warning.message = message;
        warning.byteOffset = byteOffset;
        warning.sourcePosition = position;
        result.warnings.push_back(warning);
    }

    BinaryArtLoader::LoadResult fail(
        BinaryArtLoader::FileType fileType,
        const std::string& message,
        std::size_t byteOffset = 0)
    {
        BinaryArtLoader::LoadResult result;
        result.success = false;
        result.detectedFileType = fileType;
        result.hasParseFailure = true;
        result.failingByteOffset = byteOffset;
        result.errorMessage = message;
        return result;
    }

    BinaryArtLoader::LoadResult decodeCellStream(
        std::string_view bytes,
        int width,
        const BinaryArtLoader::LoadOptions& options,
        BinaryArtLoader::FileType fileType,
        bool usedIceColors,
        const BinaryArtLoader::SauceMetadata& sauce)
    {
        BinaryArtLoader::LoadResult result;
        result.detectedFileType = fileType;
        result.usedIceColors = usedIceColors;
        result.sauce = sauce;

        if (width <= 0)
        {
            return fail(fileType, "Binary art width must be greater than zero.");
        }

        if ((bytes.size() % 2u) != 0u)
        {
            return fail(fileType, "Binary art byte stream must contain glyph/attribute pairs.");
        }

        const std::size_t cellCount = bytes.size() / 2u;
        if ((cellCount % static_cast<std::size_t>(width)) != 0u)
        {
            return fail(fileType, "Binary art cell count does not align with the resolved width.");
        }

        const int height = static_cast<int>(cellCount / static_cast<std::size_t>(width));
        TextObjectBuilder builder(width, height);

        std::size_t offset = 0;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const unsigned char glyphByte = static_cast<unsigned char>(bytes[offset++]);
                const unsigned char attrByte = static_cast<unsigned char>(bytes[offset++]);

                const std::optional<Style> style =
                    styleFromDosAttribute(attrByte, options.baseStyle, usedIceColors);

                if (!builder.setGlyph(x, y, decodeCp437Byte(glyphByte), style))
                {
                    return fail(fileType, "Failed to populate TextObjectBuilder for binary art.", offset);
                }
            }
        }

        result.object = builder.build();
        result.success = true;
        result.resolvedWidth = width;
        result.resolvedHeight = height;
        return result;
    }

    BinaryArtLoader::LoadResult decodeCompressedXBinStub(
        std::string_view bytes,
        std::size_t payloadOffset,
        int width,
        int height,
        const BinaryArtLoader::LoadOptions& options,
        const BinaryArtLoader::SauceMetadata& sauce,
        bool usedIceColors)
    {
        (void)bytes;
        (void)payloadOffset;
        (void)width;
        (void)height;
        (void)options;
        (void)sauce;
        (void)usedIceColors;

        BinaryArtLoader::LoadResult result =
            fail(BinaryArtLoader::FileType::XBin, "Compressed XBIN payload decoding hook is not implemented yet.", payloadOffset);

        addWarning(
            result,
            BinaryArtLoader::LoadWarningCode::CompressedXBinStubEncountered,
            "Compressed XBIN payload was detected and routed to the explicit stub hook.",
            payloadOffset,
            {});

        return result;
    }

    BinaryArtLoader::LoadResult loadBin(
        std::string_view bytes,
        const BinaryArtLoader::LoadOptions& options,
        const BinaryArtLoader::SauceMetadata& sauce)
    {
        int width = options.defaultColumns;
        if (options.preferSauceWidth && sauce.present && sauce.tInfo1 > 0)
        {
            width = static_cast<int>(sauce.tInfo1);
        }

        BinaryArtLoader::LoadResult result =
            decodeCellStream(bytes, width, options, BinaryArtLoader::FileType::Bin, options.enableIceColors, sauce);

        if (sauce.present)
        {
            addWarning(result, BinaryArtLoader::LoadWarningCode::SauceMetadataPresent,
                "SAUCE metadata was detected and parsed.", 0, {});
        }

        if (options.preferSauceWidth && sauce.present && sauce.tInfo1 > 0)
        {
            addWarning(result, BinaryArtLoader::LoadWarningCode::SauceWidthOverrideApplied,
                "SAUCE width metadata was applied to binary art import.", 0, {});
        }

        return result;
    }

    BinaryArtLoader::LoadResult loadAdf(
        std::string_view bytes,
        const BinaryArtLoader::LoadOptions& options,
        const BinaryArtLoader::SauceMetadata& sauce)
    {
        int width = options.defaultColumns;
        if (options.preferSauceWidth && sauce.present && sauce.tInfo1 > 0)
        {
            width = static_cast<int>(sauce.tInfo1);
        }

        BinaryArtLoader::LoadResult result =
            decodeCellStream(bytes, width, options, BinaryArtLoader::FileType::Adf, options.enableIceColors, sauce);

        if (result.success)
        {
            addWarning(result, BinaryArtLoader::LoadWarningCode::AdfSubsetAssumed,
                "ADF was loaded using the initial ArtWorx-style bin-compatible subset.", 0, {});
        }

        if (sauce.present)
        {
            addWarning(result, BinaryArtLoader::LoadWarningCode::SauceMetadataPresent,
                "SAUCE metadata was detected and parsed.", 0, {});
        }

        if (options.preferSauceWidth && sauce.present && sauce.tInfo1 > 0)
        {
            addWarning(result, BinaryArtLoader::LoadWarningCode::SauceWidthOverrideApplied,
                "SAUCE width metadata was applied to binary art import.", 0, {});
        }

        return result;
    }

    BinaryArtLoader::LoadResult loadXBin(
        std::string_view bytes,
        const BinaryArtLoader::LoadOptions& options,
        const BinaryArtLoader::SauceMetadata& sauce)
    {
        if (bytes.size() < 11)
        {
            return fail(BinaryArtLoader::FileType::XBin, "XBIN file is too small to contain a valid header.");
        }

        if (!(bytes[0] == 'X' && bytes[1] == 'B' && bytes[2] == 'I' && bytes[3] == 'N' &&
              static_cast<unsigned char>(bytes[4]) == 0x1A))
        {
            return fail(BinaryArtLoader::FileType::XBin, "Missing XBIN signature.");
        }

        bool ok = false;
        const std::uint16_t width = readLe16(bytes, 5, ok);
        if (!ok)
        {
            return fail(BinaryArtLoader::FileType::XBin, "Failed to read XBIN width.", 5);
        }

        const std::uint16_t height = readLe16(bytes, 7, ok);
        if (!ok)
        {
            return fail(BinaryArtLoader::FileType::XBin, "Failed to read XBIN height.", 7);
        }

        const unsigned char fontHeight = static_cast<unsigned char>(bytes[9]);
        const unsigned char flags = static_cast<unsigned char>(bytes[10]);

        const bool hasPalette = (flags & 0x01) != 0;
        const bool hasFont = (flags & 0x02) != 0;
        const bool isCompressed = (flags & 0x04) != 0;
        const bool nonBlinkMode = (flags & 0x08) != 0;
        const bool has512Chars = (flags & 0x10) != 0;

        BinaryArtLoader::LoadResult result;
        result.detectedFileType = BinaryArtLoader::FileType::XBin;
        result.sauce = sauce;
        result.usedIceColors = nonBlinkMode || options.enableIceColors;

        std::size_t offset = 11;

        if (hasPalette)
        {
            if (offset + 48 > bytes.size())
            {
                return fail(BinaryArtLoader::FileType::XBin, "XBIN palette section is truncated.", offset);
            }

            offset += 48;
            addWarning(result, BinaryArtLoader::LoadWarningCode::PaletteDataIgnored,
                "XBIN palette data was present but is not yet imported into engine palette state.", offset, {});
        }

        if (hasFont)
        {
            const std::size_t glyphCount = has512Chars ? 512u : 256u;
            const std::size_t fontBytes = glyphCount * static_cast<std::size_t>(fontHeight);

            if (offset + fontBytes > bytes.size())
            {
                return fail(BinaryArtLoader::FileType::XBin, "XBIN font section is truncated.", offset);
            }

            offset += fontBytes;
            addWarning(result, BinaryArtLoader::LoadWarningCode::FontDataIgnored,
                "XBIN font data was present but is not yet imported into engine glyph/font policy.", offset, {});
        }

        if (isCompressed)
        {
            if (options.xbinCompressionSupport == BinaryArtLoader::XBinCompressionSupport::StubHookOnly)
            {
                BinaryArtLoader::LoadResult stub =
                    decodeCompressedXBinStub(
                        bytes,
                        offset,
                        static_cast<int>(width),
                        static_cast<int>(height),
                        options,
                        sauce,
                        result.usedIceColors);

                stub.warnings.insert(
                    stub.warnings.begin(),
                    result.warnings.begin(),
                    result.warnings.end());

                return stub;
            }

            BinaryArtLoader::LoadResult rejected =
                fail(BinaryArtLoader::FileType::XBin, "Compressed XBIN is not supported in this build.", offset);

            addWarning(rejected, BinaryArtLoader::LoadWarningCode::CompressedXBinStubEncountered,
                "Compressed XBIN payload was detected and rejected explicitly.", offset, {});

            return rejected;
        }

        const std::size_t expectedCellBytes =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height) * 2u;

        if (offset + expectedCellBytes > bytes.size())
        {
            return fail(BinaryArtLoader::FileType::XBin, "XBIN cell payload is truncated.", offset);
        }

        BinaryArtLoader::LoadResult decoded =
            decodeCellStream(
                bytes.substr(offset, expectedCellBytes),
                static_cast<int>(width),
                options,
                BinaryArtLoader::FileType::XBin,
                result.usedIceColors,
                sauce);

        decoded.warnings.insert(
            decoded.warnings.begin(),
            result.warnings.begin(),
            result.warnings.end());

        if (sauce.present)
        {
            addWarning(decoded, BinaryArtLoader::LoadWarningCode::SauceMetadataPresent,
                "SAUCE metadata was detected and parsed.", 0, {});
        }

        if (offset + expectedCellBytes < bytes.size())
        {
            addWarning(decoded, BinaryArtLoader::LoadWarningCode::ExtraTrailingBytesIgnored,
                "Extra trailing bytes after XBIN payload were ignored.", offset + expectedCellBytes, {});
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
        const SauceRecord sauce = parseSauce(bytes);
        const std::string_view content = bytes.substr(0, sauce.contentSize);

        LoadResult result;

        if (content.size() >= 5 &&
            content[0] == 'X' &&
            content[1] == 'B' &&
            content[2] == 'I' &&
            content[3] == 'N' &&
            static_cast<unsigned char>(content[4]) == 0x1A)
        {
            result = loadXBin(content, options, sauce.metadata);
        }
        else if (options.fileType == FileType::Adf)
        {
            result = loadAdf(content, options, sauce.metadata);
        }
        else
        {
            result = loadBin(content, options, sauce.metadata);
        }

        if (sauce.truncated)
        {
            addWarning(result, LoadWarningCode::TruncatedSauceIgnored,
                "SAUCE comment metadata appeared truncated and was partially ignored.", sauce.contentSize, {});
        }

        return result;
    }

    bool hasWarning(const LoadResult& result, LoadWarningCode code)
    {
        for (const LoadWarning& warning : result.warnings)
        {
            if (warning.code == code)
            {
                return true;
            }
        }

        return false;
    }

    const LoadWarning* getWarningByCode(const LoadResult& result, LoadWarningCode code)
    {
        for (const LoadWarning& warning : result.warnings)
        {
            if (warning.code == code)
            {
                return &warning;
            }
        }

        return nullptr;
    }

    std::string formatLoadError(const LoadResult& result)
    {
        std::ostringstream stream;
        stream << "Binary art load failed";
        if (!result.errorMessage.empty())
        {
            stream << ": " << result.errorMessage;
        }

        if (result.hasParseFailure)
        {
            stream << " [byte=" << result.failingByteOffset << "]";
        }

        return stream.str();
    }

    std::string formatLoadSuccess(const LoadResult& result)
    {
        std::ostringstream stream;
        stream << "Binary art load succeeded: "
               << result.resolvedWidth << "x" << result.resolvedHeight
               << ", type=" << toString(result.detectedFileType);

        if (result.usedIceColors)
        {
            stream << ", iceColors=true";
        }

        if (result.sauce.present)
        {
            stream << ", SAUCE title=\"" << result.sauce.title << "\"";
        }

        if (!result.warnings.empty())
        {
            stream << ", warnings=" << result.warnings.size();
        }

        return stream.str();
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
        case LoadWarningCode::SauceMetadataPresent: return "SauceMetadataPresent";
        case LoadWarningCode::SauceWidthOverrideApplied: return "SauceWidthOverrideApplied";
        case LoadWarningCode::PaletteDataIgnored: return "PaletteDataIgnored";
        case LoadWarningCode::FontDataIgnored: return "FontDataIgnored";
        case LoadWarningCode::AdfSubsetAssumed: return "AdfSubsetAssumed";
        case LoadWarningCode::ExtraTrailingBytesIgnored: return "ExtraTrailingBytesIgnored";
        case LoadWarningCode::CompressedXBinStubEncountered: return "CompressedXBinStubEncountered";
        case LoadWarningCode::TruncatedSauceIgnored: return "TruncatedSauceIgnored";
        default: return "Unknown";
        }
    }

    const char* toString(XBinCompressionSupport mode)
    {
        switch (mode)
        {
        case XBinCompressionSupport::RejectCompressed: return "RejectCompressed";
        case XBinCompressionSupport::StubHookOnly: return "StubHookOnly";
        default: return "Unknown";
        }
    }
}
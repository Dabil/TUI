#include "Rendering/Objects/TextObjectExporter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

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

    TextObjectExporter::FileType resolveFileType(
        const std::string& filePath,
        const TextObjectExporter::SaveOptions& options)
    {
        if (options.fileType != TextObjectExporter::FileType::Auto)
        {
            return options.fileType;
        }

        return TextObjectExporter::detectFileType(filePath);
    }

    TextObjectExporter::Encoding resolveEncoding(
        TextObjectExporter::FileType fileType,
        const TextObjectExporter::SaveOptions& options,
        std::string& outError)
    {
        if (options.encoding != TextObjectExporter::Encoding::Auto)
        {
            return options.encoding;
        }

        if (fileType == TextObjectExporter::FileType::Nfo)
        {
            outError = "NFO export requires explicit encoding. Use Encoding::Cp437 or another deliberate choice.";
            return TextObjectExporter::Encoding::Auto;
        }

        return TextObjectExporter::Encoding::Utf8;
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

    bool tryEncodeAscii(
        char32_t codePoint,
        char replacementChar,
        bool allowLossyConversion,
        std::string& outBytes,
        bool& outLossy)
    {
        codePoint = UnicodeConversion::sanitizeCodePoint(codePoint);

        if (codePoint <= 0x7F)
        {
            outBytes.push_back(static_cast<char>(codePoint));
            return true;
        }

        if (!allowLossyConversion)
        {
            return false;
        }

        outBytes.push_back(replacementChar);
        outLossy = true;
        return true;
    }

    bool tryEncodeLatin1(
        char32_t codePoint,
        char replacementChar,
        bool allowLossyConversion,
        std::string& outBytes,
        bool& outLossy)
    {
        codePoint = UnicodeConversion::sanitizeCodePoint(codePoint);

        if (codePoint <= 0xFF)
        {
            outBytes.push_back(static_cast<char>(static_cast<unsigned char>(codePoint)));
            return true;
        }

        if (!allowLossyConversion)
        {
            return false;
        }

        outBytes.push_back(replacementChar);
        outLossy = true;
        return true;
    }

    bool tryEncodeCp437(
        char32_t codePoint,
        char replacementChar,
        bool allowLossyConversion,
        std::string& outBytes,
        bool& outLossy)
    {
        codePoint = UnicodeConversion::sanitizeCodePoint(codePoint);

        if (codePoint <= 0x7F)
        {
            outBytes.push_back(static_cast<char>(codePoint));
            return true;
        }

        for (int i = 0; i < 128; ++i)
        {
            const unsigned char candidate = static_cast<unsigned char>(0x80 + i);
            if (decodeCp437Byte(candidate) == codePoint)
            {
                outBytes.push_back(static_cast<char>(candidate));
                return true;
            }
        }

        if (!allowLossyConversion)
        {
            return false;
        }

        outBytes.push_back(replacementChar);
        outLossy = true;
        return true;
    }

    std::u32string buildLineText(
        const TextObject& object,
        int row,
        bool preserveTrailingSpaces)
    {
        std::u32string line;
        line.reserve(static_cast<std::size_t>(object.getWidth()));

        for (int x = 0; x < object.getWidth(); ++x)
        {
            const TextObjectCell& cell = object.getCell(x, row);

            if (cell.kind == CellKind::WideTrailing ||
                cell.kind == CellKind::CombiningContinuation)
            {
                continue;
            }

            if (cell.kind == CellKind::Empty)
            {
                line.push_back(U' ');
                continue;
            }

            line.push_back(UnicodeConversion::sanitizeCodePoint(cell.glyph));
        }

        if (!preserveTrailingSpaces)
        {
            while (!line.empty() && line.back() == U' ')
            {
                line.pop_back();
            }
        }

        return line;
    }

    std::u32string buildPlainText(
        const TextObject& object,
        const TextObjectExporter::SaveOptions& options)
    {
        std::u32string text;

        if (!object.isLoaded())
        {
            return text;
        }

        const int height = object.getHeight();

        for (int row = 0; row < height; ++row)
        {
            std::u32string line = buildLineText(object, row, options.preserveTrailingSpaces);
            text.append(line);

            if (row + 1 < height)
            {
                text.push_back(U'\n');
            }
        }

        return text;
    }

    bool tryEncodeText(
        std::u32string_view text,
        TextObjectExporter::Encoding encoding,
        const TextObjectExporter::SaveOptions& options,
        std::string& outBytes,
        std::size_t& outLossyCount)
    {
        outBytes.clear();
        outLossyCount = 0;

        if (encoding == TextObjectExporter::Encoding::Utf8)
        {
            outBytes = UnicodeConversion::u32ToUtf8(text);
            return true;
        }

        outBytes.reserve(text.size());

        for (char32_t codePoint : text)
        {
            bool lossyForThisCodePoint = false;
            bool success = false;

            switch (encoding)
            {
            case TextObjectExporter::Encoding::Ascii:
                success = tryEncodeAscii(
                    codePoint,
                    options.replacementChar,
                    options.allowLossyConversion,
                    outBytes,
                    lossyForThisCodePoint);
                break;

            case TextObjectExporter::Encoding::Latin1:
                success = tryEncodeLatin1(
                    codePoint,
                    options.replacementChar,
                    options.allowLossyConversion,
                    outBytes,
                    lossyForThisCodePoint);
                break;

            case TextObjectExporter::Encoding::Cp437:
                success = tryEncodeCp437(
                    codePoint,
                    options.replacementChar,
                    options.allowLossyConversion,
                    outBytes,
                    lossyForThisCodePoint);
                break;

            case TextObjectExporter::Encoding::Auto:
            case TextObjectExporter::Encoding::Utf8:
            default:
                success = false;
                break;
            }

            if (!success)
            {
                return false;
            }

            if (lossyForThisCodePoint)
            {
                ++outLossyCount;
            }
        }

        return true;
    }

    std::string applyLineEndings(
        std::string_view bytes,
        TextObjectExporter::LineEnding lineEnding)
    {
        if (lineEnding == TextObjectExporter::LineEnding::Lf)
        {
            return std::string(bytes);
        }

        std::string result;
        result.reserve(bytes.size());

        for (char ch : bytes)
        {
            if (ch == '\n')
            {
                result.push_back('\r');
                result.push_back('\n');
                continue;
            }

            result.push_back(ch);
        }

        return result;
    }
}

namespace TextObjectExporter
{
    FileType detectFileType(const std::string& filePath)
    {
        const std::string ext = getFileExtensionLower(filePath);

        if (ext == ".txt")
        {
            return FileType::Txt;
        }

        if (ext == ".asc")
        {
            return FileType::Asc;
        }

        if (ext == ".diz")
        {
            return FileType::Diz;
        }

        if (ext == ".nfo")
        {
            return FileType::Nfo;
        }

        return FileType::Unknown;
    }

    SaveResult exportToBytes(const TextObject& object, const SaveOptions& options)
    {
        SaveResult result;
        result.resolvedFileType = options.fileType == FileType::Auto
            ? FileType::Unknown
            : options.fileType;

        if (!object.isLoaded())
        {
            result.errorMessage = "Cannot export an unloaded TextObject.";
            return result;
        }

        std::string encodingError;
        result.resolvedEncoding = resolveEncoding(result.resolvedFileType, options, encodingError);
        if (result.resolvedEncoding == Encoding::Auto)
        {
            result.errorMessage = encodingError;
            return result;
        }

        const std::u32string plainText = buildPlainText(object, options);

        std::string encodedBytes;
        std::size_t lossyCount = 0;
        if (!tryEncodeText(plainText, result.resolvedEncoding, options, encodedBytes, lossyCount))
        {
            result.errorMessage =
                "Export failed because the TextObject contains code points that cannot be represented "
                "in the selected encoding without lossy conversion.";
            return result;
        }

        result.hadLossyConversion = (lossyCount > 0);
        result.lossyCodePointCount = lossyCount;

        result.bytes = applyLineEndings(encodedBytes, options.lineEnding);

        if (result.resolvedEncoding == Encoding::Utf8 && options.includeUtf8Bom)
        {
            result.bytes.insert(0, "\xEF\xBB\xBF");
            result.usedUtf8Bom = true;
        }

        result.success = true;
        return result;
    }

    SaveResult saveToFile(const TextObject& object, const std::string& filePath, const SaveOptions& options)
    {
        SaveOptions resolvedOptions = options;

        if (resolvedOptions.fileType == FileType::Auto)
        {
            resolvedOptions.fileType = detectFileType(filePath);
        }

        SaveResult result = exportToBytes(object, resolvedOptions);
        result.resolvedFileType = resolvedOptions.fileType;

        if (!result.success)
        {
            return result;
        }

        std::ofstream file(filePath, std::ios::binary);
        if (!file)
        {
            result.success = false;
            result.errorMessage = "Failed to open output file for writing.";
            result.bytes.clear();
            return result;
        }

        file.write(result.bytes.data(), static_cast<std::streamsize>(result.bytes.size()));
        if (!file)
        {
            result.success = false;
            result.errorMessage = "Failed while writing output file.";
            result.bytes.clear();
            return result;
        }

        return result;
    }

    bool trySaveToFile(const TextObject& object, const std::string& filePath)
    {
        return trySaveToFile(object, filePath, SaveOptions{});
    }

    bool trySaveToFile(const TextObject& object, const std::string& filePath, const SaveOptions& options)
    {
        return saveToFile(object, filePath, options).success;
    }

    const char* toString(FileType fileType)
    {
        switch (fileType)
        {
        case FileType::Auto:
            return "Auto";
        case FileType::Unknown:
            return "Unknown";
        case FileType::Txt:
            return "TXT";
        case FileType::Asc:
            return "ASC";
        case FileType::Diz:
            return "DIZ";
        case FileType::Nfo:
            return "NFO";
        default:
            return "Unknown";
        }
    }

    const char* toString(Encoding encoding)
    {
        switch (encoding)
        {
        case Encoding::Auto:
            return "Auto";
        case Encoding::Utf8:
            return "UTF-8";
        case Encoding::Ascii:
            return "ASCII";
        case Encoding::Latin1:
            return "Latin-1";
        case Encoding::Cp437:
            return "CP437";
        default:
            return "Unknown";
        }
    }

    const char* toString(LineEnding lineEnding)
    {
        switch (lineEnding)
        {
        case LineEnding::Lf:
            return "LF";
        case LineEnding::CrLf:
            return "CRLF";
        default:
            return "Unknown";
        }
    }
}
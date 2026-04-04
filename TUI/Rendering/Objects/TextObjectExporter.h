#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "Rendering/Objects/TextObject.h"

namespace TextObjectExporter
{
    enum class FileType
    {
        Auto,
        Unknown,
        Txt,
        Asc,
        Diz,
        Nfo
    };

    enum class Encoding
    {
        Auto,
        Utf8,
        Ascii,
        Latin1,
        Cp437
    };

    enum class LineEnding
    {
        Lf,
        CrLf
    };

    struct SaveOptions
    {
        FileType fileType = FileType::Auto;
        Encoding encoding = Encoding::Auto;

        LineEnding lineEnding = LineEnding::Lf;

        bool includeUtf8Bom = false;
        bool preserveTrailingSpaces = true;

        bool allowLossyConversion = false;
        char replacementChar = '?';
    };

    struct SaveResult
    {
        bool success = false;

        FileType resolvedFileType = FileType::Unknown;
        Encoding resolvedEncoding = Encoding::Utf8;

        bool usedUtf8Bom = false;
        bool hadLossyConversion = false;
        std::size_t lossyCodePointCount = 0;

        std::string bytes;
        std::string errorMessage;
    };

    FileType detectFileType(const std::string& filePath);

    SaveResult exportToBytes(const TextObject& object, const SaveOptions& options = {});
    SaveResult saveToFile(const TextObject& object, const std::string& filePath, const SaveOptions& options = {});

    bool trySaveToFile(const TextObject& object, const std::string& filePath);
    bool trySaveToFile(const TextObject& object, const std::string& filePath, const SaveOptions& options);

    const char* toString(FileType fileType);
    const char* toString(Encoding encoding);
    const char* toString(LineEnding lineEnding);
}
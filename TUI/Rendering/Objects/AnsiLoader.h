#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Rendering/Objects/TextObject.h"
#include "Rendering/Styles/Style.h"

namespace AnsiLoader
{
    enum class FileType
    {
        Auto,
        Unknown,
        Ans
    };

    enum class LoadWarningCode
    {
        None,
        UnsupportedSequenceIgnored,
        UnsupportedPrivateSequenceIgnored,
        CursorClamped,
        CanvasExpanded,
        BareEscapeIgnored
    };

    struct LoadWarning
    {
        LoadWarningCode code = LoadWarningCode::None;
        std::string message;

        bool isValid() const
        {
            return code != LoadWarningCode::None;
        }
    };

    struct LoadOptions
    {
        FileType fileType = FileType::Auto;

        int defaultColumns = 80;
        int maxColumns = 4096;
        int maxRows = 4096;

        bool strictUnsupportedCommands = false;
        bool clampCursorToCanvas = true;
        bool expandCanvasOnDemand = true;
        bool wrapAtColumnLimit = true;
        bool treatBareLfAsNewLine = true;
        bool preserveTrailingSpaces = true;

        std::optional<Style> baseStyle;
    };

    struct LoadResult
    {
        TextObject object;
        bool success = false;

        FileType detectedFileType = FileType::Unknown;

        int resolvedWidth = 0;
        int resolvedHeight = 0;

        std::vector<LoadWarning> warnings;
        std::string errorMessage;
    };

    FileType detectFileType(const std::string& filePath);

    LoadResult loadFromFile(const std::string& filePath);
    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options);
    LoadResult loadFromFile(const std::string& filePath, const Style& style);

    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject);
    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const LoadOptions& options);
    bool tryLoadFromFile(const std::string& filePath, TextObject& outObject, const Style& style);

    LoadResult loadFromBytes(std::string_view bytes, const LoadOptions& options = {});

    const char* toString(FileType fileType);
    const char* toString(LoadWarningCode warningCode);
}
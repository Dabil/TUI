#include "Rendering/Objects/XpSequenceLoader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using namespace XpSequenceLoader;

    struct ParsedFrameLine
    {
        int lineNumber = 0;
        int frameIndex = -1;
        std::string sourcePath;
        std::string label;
        XpArtLoader::XpFrameOverrides overrides;
    };

    std::string trimCopy(std::string_view value)
    {
        std::size_t begin = 0;
        while (begin < value.size() &&
            std::isspace(static_cast<unsigned char>(value[begin])))
        {
            ++begin;
        }

        std::size_t end = value.size();
        while (end > begin &&
            std::isspace(static_cast<unsigned char>(value[end - 1])))
        {
            --end;
        }

        return std::string(value.substr(begin, end - begin));
    }

    bool startsWith(std::string_view value, std::string_view prefix)
    {
        return value.size() >= prefix.size() &&
            value.substr(0, prefix.size()) == prefix;
    }

    std::string unquote(std::string value)
    {
        if (value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"')
        {
            value.erase(value.begin());
            value.pop_back();
        }
        return value;
    }

    std::vector<std::string> tokenizeFrameArguments(std::string_view text)
    {
        std::vector<std::string> tokens;
        std::string current;
        bool inQuotes = false;

        for (char ch : text)
        {
            if (ch == '"')
            {
                inQuotes = !inQuotes;
                current.push_back(ch);
                continue;
            }

            if (!inQuotes && std::isspace(static_cast<unsigned char>(ch)))
            {
                if (!current.empty())
                {
                    tokens.push_back(current);
                    current.clear();
                }
                continue;
            }

            current.push_back(ch);
        }

        if (!current.empty())
        {
            tokens.push_back(current);
        }

        return tokens;
    }

    void addDiagnostic(
        XpSequenceLoader::LoadResult& result,
        DiagnosticCode code,
        const std::string& message,
        int lineNumber,
        int frameIndex,
        const std::string& sourcePath = std::string(),
        bool isError = true)
    {
        Diagnostic diagnostic;
        diagnostic.code = code;
        diagnostic.isError = isError;
        diagnostic.lineNumber = lineNumber;
        diagnostic.frameIndex = frameIndex;
        diagnostic.manifestPath = result.manifestPath;
        diagnostic.sourcePath = sourcePath;
        diagnostic.message = message;
        result.diagnostics.push_back(std::move(diagnostic));
    }

    XpSequenceLoader::LoadResult failResult(
        const std::string& manifestPath,
        DiagnosticCode code,
        const std::string& message,
        int lineNumber,
        int frameIndex,
        const std::string& sourcePath = std::string())
    {
        XpSequenceLoader::LoadResult result;
        result.manifestPath = manifestPath;
        result.errorMessage = message;
        addDiagnostic(result, code, message, lineNumber, frameIndex, sourcePath, true);
        return result;
    }

    bool tryParseInt(const std::string& text, int& outValue)
    {
        try
        {
            std::size_t consumed = 0;
            const int value = std::stoi(text, &consumed, 10);
            if (consumed != text.size())
            {
                return false;
            }

            outValue = value;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool tryParseCompositeMode(
        const std::string& text,
        XpArtLoader::XpCompositeMode& outValue)
    {
        if (text == "Phase45BCompatible")
        {
            outValue = XpArtLoader::XpCompositeMode::Phase45BCompatible;
            return true;
        }

        if (text == "StrictOpaqueOverwrite")
        {
            outValue = XpArtLoader::XpCompositeMode::StrictOpaqueOverwrite;
            return true;
        }

        return false;
    }

    bool tryParseVisibleLayerMode(
        const std::string& text,
        XpArtLoader::XpVisibleLayerMode& outValue)
    {
        if (text == "UseDocumentVisibility")
        {
            outValue = XpArtLoader::XpVisibleLayerMode::UseDocumentVisibility;
            return true;
        }

        if (text == "ForceAllLayersVisible")
        {
            outValue = XpArtLoader::XpVisibleLayerMode::ForceAllLayersVisible;
            return true;
        }

        if (text == "UseExplicitVisibleLayerList")
        {
            outValue = XpArtLoader::XpVisibleLayerMode::UseExplicitVisibleLayerList;
            return true;
        }

        return false;
    }

    bool tryParseExplicitVisibleLayerList(
        const std::string& text,
        std::vector<int>& outIndices)
    {
        outIndices.clear();
        const std::string trimmed = trimCopy(text);
        if (trimmed.empty())
        {
            return false;
        }

        std::stringstream stream(trimmed);
        std::string item;
        while (std::getline(stream, item, ','))
        {
            const std::string part = trimCopy(item);
            int index = -1;
            if (!tryParseInt(part, index) || index < 0)
            {
                outIndices.clear();
                return false;
            }

            outIndices.push_back(index);
        }

        return !outIndices.empty();
    }

    bool readAllText(const std::string& filePath, std::string& outText)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file)
        {
            return false;
        }

        std::ostringstream stream;
        stream << file.rdbuf();
        outText = stream.str();
        return file.good() || file.eof();
    }

    std::filesystem::path resolveFramePath(
        const std::filesystem::path& manifestPath,
        const std::string& sourcePath)
    {
        const std::filesystem::path candidate(sourcePath);
        if (candidate.is_absolute())
        {
            return candidate.lexically_normal();
        }

        const std::filesystem::path baseDirectory =
            manifestPath.has_parent_path()
            ? manifestPath.parent_path()
            : std::filesystem::current_path();

        return (baseDirectory / candidate).lexically_normal();
    }

    bool parseAssignment(
        const std::string& line,
        std::string& outKey,
        std::string& outValue)
    {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos)
        {
            return false;
        }

        outKey = trimCopy(line.substr(0, eq));
        outValue = trimCopy(line.substr(eq + 1));
        outValue = unquote(outValue);
        return !outKey.empty();
    }

    bool parseFrameLine(
        const std::string& line,
        int lineNumber,
        XpSequenceLoader::LoadResult& ioResult,
        ParsedFrameLine& outFrame)
    {
        outFrame = ParsedFrameLine{};
        outFrame.lineNumber = lineNumber;

        const std::string remainder = trimCopy(std::string_view(line).substr(5));
        const std::vector<std::string> tokens = tokenizeFrameArguments(remainder);
        if (tokens.empty())
        {
            addDiagnostic(
                ioResult,
                DiagnosticCode::InvalidFrameDirective,
                "Frame directive is missing key=value arguments.",
                lineNumber,
                -1);
            return false;
        }

        for (const std::string& token : tokens)
        {
            const std::size_t eq = token.find('=');
            if (eq == std::string::npos)
            {
                addDiagnostic(
                    ioResult,
                    DiagnosticCode::InvalidFrameDirective,
                    "Frame directive token is missing '=': " + token,
                    lineNumber,
                    outFrame.frameIndex);
                return false;
            }

            const std::string key = trimCopy(token.substr(0, eq));
            std::string value = trimCopy(token.substr(eq + 1));
            value = unquote(value);

            if (key == "index")
            {
                if (!tryParseInt(value, outFrame.frameIndex) || outFrame.frameIndex < 0)
                {
                    addDiagnostic(
                        ioResult,
                        DiagnosticCode::InvalidFrameIndex,
                        "Frame index must be a non-negative integer.",
                        lineNumber,
                        outFrame.frameIndex);
                    return false;
                }
            }
            else if (key == "source")
            {
                outFrame.sourcePath = value;
            }
            else if (key == "label")
            {
                outFrame.label = value;
            }
            else if (key == "duration_ms")
            {
                int duration = 0;
                if (!tryParseInt(value, duration) || duration < 0)
                {
                    addDiagnostic(
                        ioResult,
                        DiagnosticCode::InvalidDurationMilliseconds,
                        "Frame duration_ms must be a non-negative integer.",
                        lineNumber,
                        outFrame.frameIndex);
                    return false;
                }

                outFrame.overrides.durationMilliseconds = duration;
            }
            else if (key == "composite")
            {
                XpArtLoader::XpCompositeMode compositeMode;
                if (!tryParseCompositeMode(value, compositeMode))
                {
                    addDiagnostic(
                        ioResult,
                        DiagnosticCode::InvalidCompositeMode,
                        "Invalid frame composite value: " + value,
                        lineNumber,
                        outFrame.frameIndex);
                    return false;
                }

                outFrame.overrides.compositeMode = compositeMode;
            }
            else if (key == "visible_layers")
            {
                XpArtLoader::XpVisibleLayerMode visibleLayerMode;
                if (!tryParseVisibleLayerMode(value, visibleLayerMode))
                {
                    addDiagnostic(
                        ioResult,
                        DiagnosticCode::InvalidVisibleLayerMode,
                        "Invalid frame visible_layers value: " + value,
                        lineNumber,
                        outFrame.frameIndex);
                    return false;
                }

                outFrame.overrides.visibleLayerMode = visibleLayerMode;
            }
            else if (key == "explicit_visible_layers")
            {
                if (!tryParseExplicitVisibleLayerList(
                    value,
                    outFrame.overrides.explicitVisibleLayerIndices))
                {
                    addDiagnostic(
                        ioResult,
                        DiagnosticCode::InvalidExplicitVisibleLayerList,
                        "Frame explicit_visible_layers must be a comma-separated list of non-negative integers.",
                        lineNumber,
                        outFrame.frameIndex);
                    return false;
                }
            }
            else
            {
                addDiagnostic(
                    ioResult,
                    DiagnosticCode::InvalidFrameDirective,
                    "Unknown frame field: " + key,
                    lineNumber,
                    outFrame.frameIndex);
                return false;
            }
        }

        if (outFrame.frameIndex < 0)
        {
            addDiagnostic(
                ioResult,
                DiagnosticCode::InvalidFrameIndex,
                "Frame directive is missing index=...",
                lineNumber,
                -1);
            return false;
        }

        if (outFrame.sourcePath.empty())
        {
            addDiagnostic(
                ioResult,
                DiagnosticCode::MissingFrameSource,
                "Frame directive is missing source=...",
                lineNumber,
                outFrame.frameIndex);
            return false;
        }

        if (outFrame.overrides.visibleLayerMode.has_value() &&
            *outFrame.overrides.visibleLayerMode ==
            XpArtLoader::XpVisibleLayerMode::UseExplicitVisibleLayerList &&
            outFrame.overrides.explicitVisibleLayerIndices.empty())
        {
            addDiagnostic(
                ioResult,
                DiagnosticCode::InvalidExplicitVisibleLayerList,
                "Frame uses visible_layers=UseExplicitVisibleLayerList but does not define explicit_visible_layers.",
                lineNumber,
                outFrame.frameIndex);
            return false;
        }

        return true;
    }

    bool finalizeContiguousFrameValidation(
        XpSequenceLoader::LoadResult& ioResult,
        const std::vector<ParsedFrameLine>& frames)
    {
        if (frames.empty())
        {
            addDiagnostic(
                ioResult,
                DiagnosticCode::MissingFrame,
                "Manifest does not define any frames.",
                0,
                -1);
            return false;
        }

        std::set<int> indices;
        for (const ParsedFrameLine& frame : frames)
        {
            if (!indices.insert(frame.frameIndex).second)
            {
                addDiagnostic(
                    ioResult,
                    DiagnosticCode::DuplicateFrameIndex,
                    "Manifest defines a duplicate frame index.",
                    frame.lineNumber,
                    frame.frameIndex);
                return false;
            }
        }

        const int minIndex = *indices.begin();
        const int maxIndex = *indices.rbegin();

        if (minIndex != 0)
        {
            addDiagnostic(
                ioResult,
                DiagnosticCode::MissingFrame,
                "Frame indices must start at 0 for deterministic sequence ownership.",
                0,
                minIndex);
            return false;
        }

        for (int index = minIndex; index <= maxIndex; ++index)
        {
            if (indices.find(index) == indices.end())
            {
                addDiagnostic(
                    ioResult,
                    DiagnosticCode::MissingFrame,
                    "Manifest is missing frame index " + std::to_string(index) + ".",
                    0,
                    index);
                return false;
            }
        }

        return true;
    }
}

namespace XpSequenceLoader
{
    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options)
    {
        std::string manifestText;
        if (!readAllText(filePath, manifestText))
        {
            return failResult(
                filePath,
                DiagnosticCode::BadRelativePath,
                "Failed to read .xpseq manifest file: " + filePath,
                0,
                -1);
        }

        return loadFromString(manifestText, filePath, options);
    }

    LoadResult loadFromString(
        std::string_view manifestUtf8,
        const std::string& manifestPath,
        const LoadOptions& options)
    {
        LoadResult result;
        result.manifestPath = manifestPath;

        std::istringstream stream{ std::string(manifestUtf8) };
        std::string line;
        int lineNumber = 0;
        bool sawHeader = false;
        std::vector<ParsedFrameLine> parsedFrames;

        while (std::getline(stream, line))
        {
            ++lineNumber;
            const std::string trimmed = trimCopy(line);

            if (trimmed.empty() || startsWith(trimmed, "#"))
            {
                continue;
            }

            if (!sawHeader)
            {
                if (!startsWith(trimmed, "xpseq"))
                {
                    addDiagnostic(
                        result,
                        DiagnosticCode::InvalidHeader,
                        "First non-comment line must be 'xpseq 1'.",
                        lineNumber,
                        -1);
                    result.errorMessage = "Invalid .xpseq header.";
                    return result;
                }

                std::stringstream headerStream(trimmed);
                std::string magic;
                std::string versionText;
                headerStream >> magic >> versionText;

                int version = 0;
                if (magic != "xpseq" || !tryParseInt(versionText, version))
                {
                    addDiagnostic(
                        result,
                        DiagnosticCode::InvalidHeader,
                        "Malformed .xpseq header. Expected 'xpseq 1'.",
                        lineNumber,
                        -1);
                    result.errorMessage = "Malformed .xpseq header.";
                    return result;
                }

                if (version != 1)
                {
                    addDiagnostic(
                        result,
                        DiagnosticCode::UnsupportedVersion,
                        "Unsupported .xpseq version: " + std::to_string(version) + ".",
                        lineNumber,
                        -1);
                    result.errorMessage = "Unsupported .xpseq version.";
                    return result;
                }

                result.resolvedVersion = version;
                sawHeader = true;
                continue;
            }

            if (startsWith(trimmed, "frame"))
            {
                ParsedFrameLine frame;
                if (!parseFrameLine(trimmed, lineNumber, result, frame))
                {
                    result.errorMessage = "Invalid .xpseq frame directive.";
                    return result;
                }

                parsedFrames.push_back(std::move(frame));
                continue;
            }

            std::string key;
            std::string value;
            if (!parseAssignment(trimmed, key, value))
            {
                addDiagnostic(
                    result,
                    DiagnosticCode::InvalidDirective,
                    "Invalid manifest directive: " + trimmed,
                    lineNumber,
                    -1);
                result.errorMessage = "Invalid .xpseq directive.";
                return result;
            }

            if (key == "sequence_label")
            {
                result.sequence.metadata.sequenceLabel = value;
            }
            else if (key == "default_duration_ms")
            {
                int duration = 0;
                if (!tryParseInt(value, duration) || duration < 0)
                {
                    addDiagnostic(
                        result,
                        DiagnosticCode::InvalidDurationMilliseconds,
                        "default_duration_ms must be a non-negative integer.",
                        lineNumber,
                        -1);
                    result.errorMessage = "Invalid default_duration_ms.";
                    return result;
                }

                result.sequence.metadata.defaultFrameDurationMilliseconds = duration;
            }
            else if (key == "default_composite")
            {
                XpArtLoader::XpCompositeMode compositeMode;
                if (!tryParseCompositeMode(value, compositeMode))
                {
                    addDiagnostic(
                        result,
                        DiagnosticCode::InvalidCompositeMode,
                        "Invalid default_composite value: " + value,
                        lineNumber,
                        -1);
                    result.errorMessage = "Invalid default_composite.";
                    return result;
                }

                result.sequence.metadata.defaultCompositeMode = compositeMode;
            }
            else if (key == "default_visible_layers")
            {
                XpArtLoader::XpVisibleLayerMode visibleLayerMode;
                if (!tryParseVisibleLayerMode(value, visibleLayerMode))
                {
                    addDiagnostic(
                        result,
                        DiagnosticCode::InvalidVisibleLayerMode,
                        "Invalid default_visible_layers value: " + value,
                        lineNumber,
                        -1);
                    result.errorMessage = "Invalid default_visible_layers.";
                    return result;
                }

                result.sequence.metadata.defaultVisibleLayerMode = visibleLayerMode;
            }
            else
            {
                addDiagnostic(
                    result,
                    DiagnosticCode::InvalidDirective,
                    "Unknown manifest directive: " + key,
                    lineNumber,
                    -1);
                result.errorMessage = "Unknown .xpseq directive.";
                return result;
            }
        }

        if (!sawHeader)
        {
            addDiagnostic(
                result,
                DiagnosticCode::EmptyManifest,
                "Manifest does not contain a valid xpseq header.",
                0,
                -1);
            result.errorMessage = "Empty or invalid .xpseq manifest.";
            return result;
        }

        if (options.requireContiguousFrameIndices &&
            !finalizeContiguousFrameValidation(result, parsedFrames))
        {
            result.errorMessage = "Frame index validation failed.";
            return result;
        }

        std::filesystem::path manifestFsPath = manifestPath.empty()
            ? std::filesystem::current_path() / "in_memory.xpseq"
            : std::filesystem::path(manifestPath);

        result.sequence.metadata.sourceManifestPath = manifestFsPath.lexically_normal().string();

        for (const ParsedFrameLine& parsedFrame : parsedFrames)
        {
            const std::filesystem::path resolvedPath =
                resolveFramePath(manifestFsPath, parsedFrame.sourcePath);

            if (parsedFrame.sourcePath.empty())
            {
                addDiagnostic(
                    result,
                    DiagnosticCode::MissingFrameSource,
                    "Frame source path is empty.",
                    parsedFrame.lineNumber,
                    parsedFrame.frameIndex);
                result.errorMessage = "Missing frame source path.";
                return result;
            }

            if (!std::filesystem::exists(resolvedPath))
            {
                addDiagnostic(
                    result,
                    DiagnosticCode::BadRelativePath,
                    "Resolved frame source does not exist: " + resolvedPath.string(),
                    parsedFrame.lineNumber,
                    parsedFrame.frameIndex,
                    resolvedPath.string());
                result.errorMessage = "Resolved frame source path does not exist.";
                return result;
            }

            XpArtLoader::LoadOptions xpOptions = options.xpLoadOptions;
            xpOptions.flattenLayers = false;

            const XpArtLoader::LoadResult xpLoadResult =
                XpArtLoader::loadFromFile(resolvedPath.string(), xpOptions);

            if (!xpLoadResult.success || !xpLoadResult.hasRetainedDocument)
            {
                addDiagnostic(
                    result,
                    DiagnosticCode::XpFrameLoadFailed,
                    xpLoadResult.errorMessage.empty()
                    ? "Failed to load referenced XP frame."
                    : xpLoadResult.errorMessage,
                    parsedFrame.lineNumber,
                    parsedFrame.frameIndex,
                    resolvedPath.string());
                result.errorMessage = "Failed to load referenced XP frame.";
                return result;
            }

            XpArtLoader::XpFrame frame;
            frame.frameIndex = parsedFrame.frameIndex;
            frame.label = parsedFrame.label;
            frame.document = std::make_shared<XpArtLoader::XpDocument>(
                xpLoadResult.retainedDocument);
            frame.overrides = parsedFrame.overrides;

            if (!frame.isValid())
            {
                addDiagnostic(
                    result,
                    DiagnosticCode::InvalidFrameDirective,
                    "Loaded frame overrides are invalid for the referenced XP document.",
                    parsedFrame.lineNumber,
                    parsedFrame.frameIndex,
                    resolvedPath.string());
                result.errorMessage = "Invalid frame overrides for referenced XP document.";
                return result;
            }

            result.sequence.frames.push_back(std::move(frame));
        }

        if (options.sortFramesByFrameIndex)
        {
            std::sort(
                result.sequence.frames.begin(),
                result.sequence.frames.end(),
                [](const XpArtLoader::XpFrame& left, const XpArtLoader::XpFrame& right)
                {
                    return left.frameIndex < right.frameIndex;
                });
        }

        if (!result.sequence.isValid())
        {
            result.errorMessage = "Constructed .xpseq retained sequence is invalid.";
            return result;
        }

        result.resolvedFrameCount = result.sequence.getFrameCount();
        result.success = true;
        return result;
    }

    const Diagnostic* getFirstDiagnosticByCode(
        const LoadResult& result,
        DiagnosticCode code)
    {
        for (const Diagnostic& diagnostic : result.diagnostics)
        {
            if (diagnostic.code == code)
            {
                return &diagnostic;
            }
        }

        return nullptr;
    }

    std::string formatLoadError(const LoadResult& result)
    {
        std::ostringstream stream;
        stream << ".xpseq load failed";

        if (!result.errorMessage.empty())
        {
            stream << ": " << result.errorMessage;
        }

        if (!result.diagnostics.empty())
        {
            const Diagnostic& diagnostic = result.diagnostics.front();
            if (diagnostic.lineNumber > 0)
            {
                stream << " (line " << diagnostic.lineNumber << ")";
            }

            if (diagnostic.frameIndex >= 0)
            {
                stream << " [frame " << diagnostic.frameIndex << "]";
            }
        }

        return stream.str();
    }

    std::string formatLoadSuccess(const LoadResult& result)
    {
        std::ostringstream stream;
        stream << ".xpseq load success: "
            << "frames=" << result.resolvedFrameCount
            << ", version=" << result.resolvedVersion;

        if (!result.sequence.metadata.sequenceLabel.empty())
        {
            stream << ", label=" << result.sequence.metadata.sequenceLabel;
        }

        if (!result.sequence.metadata.sourceManifestPath.empty())
        {
            stream << ", manifest=" << result.sequence.metadata.sourceManifestPath;
        }

        return stream.str();
    }

    const char* toString(DiagnosticCode code)
    {
        switch (code)
        {
        case DiagnosticCode::None:
            return "None";
        case DiagnosticCode::EmptyManifest:
            return "EmptyManifest";
        case DiagnosticCode::InvalidHeader:
            return "InvalidHeader";
        case DiagnosticCode::UnsupportedVersion:
            return "UnsupportedVersion";
        case DiagnosticCode::InvalidDirective:
            return "InvalidDirective";
        case DiagnosticCode::InvalidFrameDirective:
            return "InvalidFrameDirective";
        case DiagnosticCode::InvalidFrameIndex:
            return "InvalidFrameIndex";
        case DiagnosticCode::DuplicateFrameIndex:
            return "DuplicateFrameIndex";
        case DiagnosticCode::MissingFrameSource:
            return "MissingFrameSource";
        case DiagnosticCode::MissingFrame:
            return "MissingFrame";
        case DiagnosticCode::BadRelativePath:
            return "BadRelativePath";
        case DiagnosticCode::InvalidDurationMilliseconds:
            return "InvalidDurationMilliseconds";
        case DiagnosticCode::InvalidCompositeMode:
            return "InvalidCompositeMode";
        case DiagnosticCode::InvalidVisibleLayerMode:
            return "InvalidVisibleLayerMode";
        case DiagnosticCode::InvalidExplicitVisibleLayerList:
            return "InvalidExplicitVisibleLayerList";
        case DiagnosticCode::XpFrameLoadFailed:
            return "XpFrameLoadFailed";
        default:
            return "Unknown";
        }
    }
}

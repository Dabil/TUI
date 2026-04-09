#pragma once

#include <string>
#include <vector>

#include "Rendering/Objects/XpArtLoader.h"

namespace XpSequenceLoader
{
    /*
        .xpseq is the manifest-first retained XP sequence format for the
        current roadmap stage. The manifest is UTF-8 text and each frame
        continues to load through the existing .xp retained-document loader.
    */
    enum class DiagnosticCode
    {
        None,
        EmptyManifest,
        InvalidHeader,
        UnsupportedVersion,
        InvalidDirective,
        InvalidFrameDirective,
        InvalidFrameIndex,
        DuplicateFrameIndex,
        MissingFrameSource,
        MissingFrame,
        BadRelativePath,
        InvalidDurationMilliseconds,
        InvalidCompositeMode,
        InvalidVisibleLayerMode,
        InvalidExplicitVisibleLayerList,
        XpFrameLoadFailed
    };

    struct Diagnostic
    {
        DiagnosticCode code = DiagnosticCode::None;
        bool isError = true;
        int lineNumber = 0;
        int frameIndex = -1;
        std::string manifestPath;
        std::string sourcePath;
        std::string message;

        bool isValid() const
        {
            return code != DiagnosticCode::None;
        }
    };

    struct LoadOptions
    {
        XpArtLoader::LoadOptions xpLoadOptions;
        bool requireContiguousFrameIndices = true;
        bool sortFramesByFrameIndex = true;
    };

    struct LoadResult
    {
        XpArtLoader::XpSequence sequence;
        bool success = false;
        std::string manifestPath;
        int resolvedVersion = 0;
        int resolvedFrameCount = 0;
        std::vector<Diagnostic> diagnostics;
        std::string errorMessage;
    };

    LoadResult loadFromFile(const std::string& filePath, const LoadOptions& options = {});
    LoadResult loadFromString(
        std::string_view manifestUtf8,
        const std::string& manifestPath,
        const LoadOptions& options = {});

    const Diagnostic* getFirstDiagnosticByCode(
        const LoadResult& result,
        DiagnosticCode code);

    std::string formatLoadError(const LoadResult& result);
    std::string formatLoadSuccess(const LoadResult& result);

    const char* toString(DiagnosticCode code);
}

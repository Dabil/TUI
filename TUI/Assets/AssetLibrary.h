#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Assets/Loaders/AsciiBannerFontLoader.h"
#include "Assets/Models/AsciiBannerFont.h"
#include "Rendering/Objects/AnsiLoader.h"
#include "Rendering/Objects/BinaryArtLoader.h"
#include "Rendering/Objects/PlainTextLoader.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/Objects/XpArtLoader.h"
#include "Rendering/Objects/XpSequenceAccess.h"
#include "Rendering/Objects/pFontLoader.h"
#include "Utilities/AssetPaths.h"

namespace Assets
{
    struct AssetLibraryOptions
    {
        std::string assetsRoot;
        bool cacheFailures = false;

        PlainTextLoader::LoadOptions plainTextOptions;
        AnsiLoader::LoadOptions ansiOptions;
        BinaryArtLoader::LoadOptions binaryArtOptions;
        XpArtLoader::LoadOptions xpLoadOptions;
        XpArtLoader::XpFrameConversionOptions xpFrameConversionOptions;
        XpSequenceAccess::SequenceLoadOptions xpSequenceOptions;
        AsciiBannerFontLoader::LoadOptions bannerFontLoadOptions;
        PseudoFont::LoadOptions pFontLoadOptions;
    };

    struct LoadedTextAsset
    {
        std::shared_ptr<TextObject> object;
        AssetPaths::AssetType assetType = AssetPaths::AssetType::Unknown;
        std::string requestedReference;
        std::string assetKey;
        std::string requestedPath;
        std::string resolvedPath;
        std::string normalizedPath;
        std::string cacheKey;
    };

    struct LoadTextAssetResult
    {
        LoadedTextAsset asset;
        bool success = false;
        bool fromCache = false;
        std::string errorMessage;

        bool hasObject() const
        {
            return asset.object != nullptr;
        }
    };

    struct LoadedBannerFontAsset
    {
        std::shared_ptr<AsciiBannerFont> font;
        AssetPaths::AssetType assetType = AssetPaths::AssetType::Unknown;
        std::string requestedReference;
        std::string assetKey;
        std::string requestedPath;
        std::string resolvedPath;
        std::string normalizedPath;
        std::string cacheKey;
    };

    struct LoadBannerFontResult
    {
        LoadedBannerFontAsset asset;
        bool success = false;
        bool fromCache = false;
        std::string errorMessage;

        bool hasFont() const
        {
            return asset.font != nullptr;
        }
    };

    struct LoadedPseudoFontAsset
    {
        std::shared_ptr<PseudoFont::FontDefinition> font;
        AssetPaths::AssetType assetType = AssetPaths::AssetType::Unknown;
        std::string requestedReference;
        std::string assetKey;
        std::string requestedPath;
        std::string resolvedPath;
        std::string normalizedPath;
        std::string cacheKey;
    };

    struct LoadPseudoFontAssetResult
    {
        LoadedPseudoFontAsset asset;
        bool success = false;
        bool fromCache = false;
        std::string errorMessage;

        bool hasFont() const
        {
            return asset.font != nullptr;
        }
    };

    class AssetLibrary
    {
    public:
        explicit AssetLibrary(AssetLibraryOptions options = {});

        const AssetLibraryOptions& getOptions() const;
        void setOptions(const AssetLibraryOptions& options);

        void setAssetsRoot(const std::string& assetsRoot);
        const std::string& getAssetsRoot() const;

        void registerCanonicalAsset(const std::string& assetKey, const std::string& assetPath);
        void registerAlias(const std::string& assetName, const std::string& assetPathOrAssetKey);
        void registerAliases(
            const std::string& canonicalAssetKey,
            const std::vector<std::string>& aliases);

        bool hasAlias(const std::string& assetName) const;
        bool removeAlias(const std::string& assetName);
        void clearAliases();

        std::string getRegisteredAssetPath(const std::string& assetNameOrPath) const;
        std::string getPreferredAssetKeyForPath(const std::string& assetPath) const;

        LoadTextAssetResult loadTextAsset(const std::string& assetNameOrPath);
        LoadTextAssetResult reloadTextAsset(const std::string& assetNameOrPath);

        LoadBannerFontResult loadBannerFontAsset(const std::string& assetNameOrPath);
        LoadBannerFontResult reloadBannerFontAsset(const std::string& assetNameOrPath);

        LoadPseudoFontAssetResult loadPseudoFontAsset(const std::string& assetNameOrPath);
        LoadPseudoFontAssetResult reloadPseudoFontAsset(const std::string& assetNameOrPath);

        const std::shared_ptr<TextObject>* findCachedTextAsset(const std::string& assetNameOrPath) const;
        const std::shared_ptr<AsciiBannerFont>* findCachedBannerFontAsset(
            const std::string& assetNameOrPath) const;
        const std::shared_ptr<PseudoFont::FontDefinition>* findCachedPseudoFontAsset(
            const std::string& assetNameOrPath) const;

        bool evictCachedTextAsset(const std::string& assetNameOrPath);
        bool evictCachedBannerFontAsset(const std::string& assetNameOrPath);
        bool evictCachedPseudoFontAsset(const std::string& assetNameOrPath);

        void clear();

        std::size_t getCachedTextAssetCount() const;
        std::size_t getCachedBannerFontAssetCount() const;
        std::size_t getCachedPseudoFontAssetCount() const;
        std::size_t getAliasCount() const;

    private:
        struct TextCacheEntry
        {
            std::shared_ptr<TextObject> object;
            AssetPaths::AssetType assetType = AssetPaths::AssetType::Unknown;
            std::string requestedReference;
            std::string assetKey;
            std::string requestedPath;
            std::string resolvedPath;
            std::string normalizedPath;
            bool loadSucceeded = false;
            std::string errorMessage;
        };

        struct BannerFontCacheEntry
        {
            std::shared_ptr<AsciiBannerFont> font;
            AssetPaths::AssetType assetType = AssetPaths::AssetType::Unknown;
            std::string requestedReference;
            std::string assetKey;
            std::string requestedPath;
            std::string resolvedPath;
            std::string normalizedPath;
            bool loadSucceeded = false;
            std::string errorMessage;
        };

        struct PseudoFontCacheEntry
        {
            std::shared_ptr<PseudoFont::FontDefinition> font;
            AssetPaths::AssetType assetType = AssetPaths::AssetType::Unknown;
            std::string requestedReference;
            std::string assetKey;
            std::string requestedPath;
            std::string resolvedPath;
            std::string normalizedPath;
            bool loadSucceeded = false;
            std::string errorMessage;
        };

        struct ResolvedAssetReference
        {
            std::string requestedReference;
            std::string requestedPath;
            std::string assetKey;
        };

        LoadTextAssetResult loadTextAssetInternal(const std::string& assetNameOrPath, bool forceReload);
        LoadBannerFontResult loadBannerFontAssetInternal(const std::string& assetNameOrPath, bool forceReload);
        LoadPseudoFontAssetResult loadPseudoFontAssetInternal(
            const std::string& assetNameOrPath,
            bool forceReload);

        LoadTextAssetResult dispatchTextLoad(
            const ResolvedAssetReference& reference,
            const AssetPaths::ResolutionResult& resolution);

        LoadBannerFontResult dispatchBannerFontLoad(
            const ResolvedAssetReference& reference,
            const AssetPaths::ResolutionResult& resolution);

        LoadPseudoFontAssetResult dispatchPseudoFontLoad(
            const ResolvedAssetReference& reference,
            const AssetPaths::ResolutionResult& resolution);

        ResolvedAssetReference resolveAssetReference(const std::string& assetNameOrPath) const;
        std::string resolveAssetPathFromReference(const std::string& assetNameOrPath) const;
        std::string makeCacheKey(const std::string& assetNameOrPath) const;

        static std::string normalizeAssetKey(const std::string& assetKey);
        static std::string trimString(const std::string& value);

    private:
        AssetLibraryOptions m_options;
        std::unordered_map<std::string, std::string> m_aliases;
        std::unordered_map<std::string, std::string> m_preferredAssetKeysByPath;
        std::unordered_map<std::string, TextCacheEntry> m_textAssetCache;
        std::unordered_map<std::string, BannerFontCacheEntry> m_bannerFontCache;
        std::unordered_map<std::string, PseudoFontCacheEntry> m_pseudoFontCache;
    };
}

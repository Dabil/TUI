#pragma once

#include <cstdint>

/*
    Purpose:

    ConsoleCapabilities is a backend capability description for the active
    console/output path.

    It is not an authoring limit and it does not change logical Style or Color.
    It only describes what the backend can likely present directly or through
    renderer-managed emulation on the active backend path.

    Conservative defaults are used whenever support is uncertain.

    Important interpretation notes:

    - Supported means the active backend path can defensibly claim direct
      presentation support for that semantic feature.
    - Emulated means the renderer/backend path may intentionally simulate the
      feature without mutating authored logical style data.
    - Unsupported means the backend path is known not to provide that feature.
    - Unknown means support is host-dependent, ambiguous, partially simulated,
      or otherwise not strong enough to promise as direct support.

    preserveStyleSafeFallback means the backend contract expects conservative,
    style-preserving fallback behavior when authored intent cannot be presented
    directly. This is about safe presentation fallback on the backend path,
    not about reducing authoring expressiveness.

    optionalBackendFlags is a reserved extension point for future alternate
    backends. A value of zero means no extra backend-specific capability flags
    are currently advertised.

    For the current Win32 attribute renderer, Unknown should be preferred over
    Supported whenever the visible result depends on host quirks or only loosely
    resembles the authored semantic intent.
*/

enum class ConsoleColorTier
{
    None = 0,
    Basic16,
    Indexed256,
    TrueColor
};

enum class ConsoleFeatureSupport
{
    Unsupported = 0,
    Supported,
    Emulated,
    Unknown
};

enum class ConsoleBackendExtensionFlag : std::uint32_t
{
    None = 0
};

struct ConsoleCapabilities
{
    bool virtualTerminalProcessing = false;
    bool unicodeOutput = true;
    bool preserveStyleSafeFallback = true;

    std::uint32_t optionalBackendFlags = 0;

    ConsoleColorTier colorTier = ConsoleColorTier::Basic16;

    ConsoleFeatureSupport bold = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport dim = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport underline = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport reverse = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport invisible = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport strike = ConsoleFeatureSupport::Unsupported;
    ConsoleFeatureSupport slowBlink = ConsoleFeatureSupport::Emulated;
    ConsoleFeatureSupport fastBlink = ConsoleFeatureSupport::Emulated;

    static ConsoleCapabilities Conservative();
    static ConsoleCapabilities BasicWin32();
    static ConsoleCapabilities VirtualTerminal();

    bool supportsBasicColors() const;
    bool supportsIndexed256Colors() const;
    bool supportsTrueColor() const;

    bool supportsBoldDirect() const;
    bool supportsDimDirect() const;
    bool supportsUnderlineDirect() const;
    bool supportsReverseDirect() const;
    bool supportsInvisibleDirect() const;
    bool supportsStrikeDirect() const;
    bool supportsSlowBlinkDirect() const;
    bool supportsFastBlinkDirect() const;

    bool mayEmulateSlowBlink() const;
    bool mayEmulateFastBlink() const;

    bool usesPreserveStyleSafeFallback() const;
    bool hasOptionalBackendFlags() const;
    bool hasOptionalBackendFlag(ConsoleBackendExtensionFlag flag) const;
};

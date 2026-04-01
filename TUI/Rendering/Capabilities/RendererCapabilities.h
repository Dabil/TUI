#pragma once

#include <cstdint>

/*
    Purpose:

    RendererCapabilities is a backend capability description for the active
    renderer/output path.

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

    colorTier remains the broad color-family tier for the backend path.
    brightBasicColors makes the bright subset of authored basic colors explicit
    rather than leaving that distinction only implied by Basic16 handling or
    by Color::Basic enum values.

    For the current Win32 attribute renderer, Unknown should be preferred over
    Supported whenever the visible result depends on host quirks or only loosely
    resembles the authored semantic intent.
*/

/*
    Model we are building toward:

    Renderer
        declares what it knows how to do

    CapabilityDetector
        probes host/backend facts

    RendererCapabilityBuilder
        combines both into effective RendererCapabilities

    StylePolicy / ColorResolver
        make decisions from that profile

    Renderer
        executes resolved output only
*/

enum class RendererColorTier
{
    None = 0,
    Basic16,
    Indexed256,
    TrueColor
};

enum class RendererFeatureSupport
{
    Unsupported = 0,
    Supported,
    Emulated,
    Unknown
};

enum class RendererBackendExtensionFlag : std::uint32_t
{
    None = 0
};

struct RendererCapabilities
{
    bool virtualTerminalProcessing = false;
    bool unicodeOutput = true;
    bool preserveStyleSafeFallback = true;

    std::uint32_t optionalBackendFlags = 0;

    RendererColorTier colorTier = RendererColorTier::Basic16;

    /*
        Explicit support for authored bright basic colors such as
        BrightRed/BrightBlue/BrightWhite.

        This is intentionally separate from bold/dim text semantics.
        Bright basic colors describe palette/intensity color presentation,
        not text weight.
    */
    RendererFeatureSupport brightBasicColors = RendererFeatureSupport::Supported;

    RendererFeatureSupport bold = RendererFeatureSupport::Unknown;
    RendererFeatureSupport dim = RendererFeatureSupport::Unknown;
    RendererFeatureSupport underline = RendererFeatureSupport::Unknown;
    RendererFeatureSupport reverse = RendererFeatureSupport::Unknown;
    RendererFeatureSupport invisible = RendererFeatureSupport::Unknown;
    RendererFeatureSupport strike = RendererFeatureSupport::Unsupported;
    RendererFeatureSupport slowBlink = RendererFeatureSupport::Emulated;
    RendererFeatureSupport fastBlink = RendererFeatureSupport::Emulated;

    static RendererCapabilities Conservative();
    static RendererCapabilities BasicWin32();
    static RendererCapabilities VirtualTerminal();

    bool supportsBasicColors() const;
    bool supportsBrightBasicColors() const;
    bool supportsIndexed256Colors() const;
    bool supportsTrueColor() const;

    bool supportsBrightBasicColorsDirect() const;

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
    bool hasOptionalBackendFlag(RendererBackendExtensionFlag flag) const;
};
#include "Rendering/Capabilities/ConsoleCapabilities.h"

/*

how ConsoleRenderer and diagnostics should consume this data

ConsoleRenderer should use this type in two steps:

ConsoleCapabilities capabilities = ConsoleCapabilities::Conservative();

Then during initialization:

detect whether VT processing was actually enabled
populate a ConsoleCapabilities instance
build StylePolicy from that capability data
keep the capability object available for diagnostics and future reporting

The practical flow should be:

ConsoleCapabilities
    -> StylePolicy
    -> resolved Style
    -> backend mapping/output

Recommended usage patterns:

ConsoleRenderer should store one ConsoleCapabilities m_capabilities;
style mapping code should query helpers like:
supportsTrueColor()
supportsIndexed256Colors()
supportsUnderlineDirect()
usesPreserveStyleSafeFallback()
mayEmulateSlowBlink()

optionalBackendFlags is intentionally a simple reserved extension point.
The current code should not invent behavior from unknown flag bits.
It should only surface them for diagnostics until future backends define them.

diagnostics screens should report both the raw flags and the resulting
renderer behavior:
VT enabled or not
color tier
preserve-style-safe fallback behavior
optional backend flags
direct support vs unsupported vs emulated vs unknown

For the current backend, BasicWin32 must stay conservative:

- Basic16 color support is real and directly usable
- decorative/style semantics such as bold, dim, underline, reverse, and
  invisible are not safe to promise as direct support across hosts
- blink is not directly provided by the Win32 attribute path, but the current
  renderer may intentionally emulate it
- if the renderer later chooses to approximate a visual effect, that should
  be reflected in runtime adaptation data rather than overstated here

Only use VirtualTerminal() once you truly add a VT output path rather than
merely enabling the mode flag.

That keeps the model honest, conservative, and reusable.

*/

ConsoleCapabilities ConsoleCapabilities::Conservative()
{
    ConsoleCapabilities capabilities;

    capabilities.virtualTerminalProcessing = false;
    capabilities.unicodeOutput = true;
    capabilities.preserveStyleSafeFallback = true;
    capabilities.optionalBackendFlags = 0;
    capabilities.colorTier = ConsoleColorTier::Basic16;

    capabilities.bold = ConsoleFeatureSupport::Unknown;
    capabilities.dim = ConsoleFeatureSupport::Unknown;
    capabilities.underline = ConsoleFeatureSupport::Unknown;
    capabilities.reverse = ConsoleFeatureSupport::Unknown;
    capabilities.invisible = ConsoleFeatureSupport::Unknown;
    capabilities.strike = ConsoleFeatureSupport::Unsupported;
    capabilities.slowBlink = ConsoleFeatureSupport::Emulated;
    capabilities.fastBlink = ConsoleFeatureSupport::Emulated;

    return capabilities;
}

ConsoleCapabilities ConsoleCapabilities::BasicWin32()
{
    ConsoleCapabilities capabilities = Conservative();

    capabilities.virtualTerminalProcessing = false;
    capabilities.preserveStyleSafeFallback = true;
    capabilities.optionalBackendFlags = 0;
    capabilities.colorTier = ConsoleColorTier::Basic16;

    /*
        The current renderer writes through Win32 console attributes.

        That path can defensibly claim direct Basic16 color presentation, but
        several richer style semantics are not reliable enough to advertise as
        direct support:

        - bold/dim are often approximated through intensity rather than true
          weight/brightness semantics
        - underline depends on host/font behavior and is not broadly reliable
        - reverse may be visually approximated by swapping colors, but that is
          not the same as guaranteed direct backend semantic support
        - invisible can be approximated visually, but true invisibility is not
          something this path should promise

        These remain Unknown to force conservative renderer policy.

        Blink is modeled as Emulated rather than Unsupported because the active
        renderer/backend path can intentionally simulate blink timing without
        mutating authored logical style data.
    */
    capabilities.bold = ConsoleFeatureSupport::Unknown;
    capabilities.dim = ConsoleFeatureSupport::Unknown;
    capabilities.underline = ConsoleFeatureSupport::Unknown;
    capabilities.reverse = ConsoleFeatureSupport::Unknown;
    capabilities.invisible = ConsoleFeatureSupport::Unknown;
    capabilities.strike = ConsoleFeatureSupport::Unsupported;
    capabilities.slowBlink = ConsoleFeatureSupport::Emulated;
    capabilities.fastBlink = ConsoleFeatureSupport::Emulated;

    return capabilities;
}

ConsoleCapabilities ConsoleCapabilities::VirtualTerminal()
{
    ConsoleCapabilities capabilities = Conservative();

    capabilities.virtualTerminalProcessing = true;
    capabilities.preserveStyleSafeFallback = true;
    capabilities.optionalBackendFlags = 0;
    capabilities.colorTier = ConsoleColorTier::TrueColor;

    /*
        Even when VT processing is enabled, exact behavior can still vary by
        host and font environment. These defaults describe intended capability,
        not a guarantee that every terminal host behaves identically.

        This profile is meant for a future true VT presentation path, not the
        current Win32-attribute renderer.
    */
    capabilities.bold = ConsoleFeatureSupport::Supported;
    capabilities.dim = ConsoleFeatureSupport::Supported;
    capabilities.underline = ConsoleFeatureSupport::Supported;
    capabilities.reverse = ConsoleFeatureSupport::Supported;
    capabilities.invisible = ConsoleFeatureSupport::Supported;
    capabilities.strike = ConsoleFeatureSupport::Supported;
    capabilities.slowBlink = ConsoleFeatureSupport::Unknown;
    capabilities.fastBlink = ConsoleFeatureSupport::Unknown;

    return capabilities;
}

bool ConsoleCapabilities::supportsBasicColors() const
{
    return colorTier >= ConsoleColorTier::Basic16;
}

bool ConsoleCapabilities::supportsIndexed256Colors() const
{
    return colorTier >= ConsoleColorTier::Indexed256;
}

bool ConsoleCapabilities::supportsTrueColor() const
{
    return colorTier >= ConsoleColorTier::TrueColor;
}

bool ConsoleCapabilities::supportsBoldDirect() const
{
    return bold == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::supportsDimDirect() const
{
    return dim == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::supportsUnderlineDirect() const
{
    return underline == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::supportsReverseDirect() const
{
    return reverse == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::supportsInvisibleDirect() const
{
    return invisible == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::supportsStrikeDirect() const
{
    return strike == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::supportsSlowBlinkDirect() const
{
    return slowBlink == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::supportsFastBlinkDirect() const
{
    return fastBlink == ConsoleFeatureSupport::Supported;
}

bool ConsoleCapabilities::mayEmulateSlowBlink() const
{
    return slowBlink == ConsoleFeatureSupport::Emulated;
}

bool ConsoleCapabilities::mayEmulateFastBlink() const
{
    return fastBlink == ConsoleFeatureSupport::Emulated;
}

bool ConsoleCapabilities::usesPreserveStyleSafeFallback() const
{
    return preserveStyleSafeFallback;
}

bool ConsoleCapabilities::hasOptionalBackendFlags() const
{
    return optionalBackendFlags != 0;
}

bool ConsoleCapabilities::hasOptionalBackendFlag(ConsoleBackendExtensionFlag flag) const
{
    const std::uint32_t mask = static_cast<std::uint32_t>(flag);
    return (optionalBackendFlags & mask) != 0;
}

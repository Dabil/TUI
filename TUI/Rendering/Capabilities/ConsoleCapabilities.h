#pragma once

/*
    Purpose:

    ConsoleCapabilities is a backend capability description for the active
    console/output path.

    It is not an authoring limit and it does not change logical Style or Color.
    It only describes what the backend can likely present directly.

    Conservative defaults are used whenever support is uncertain.
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

struct ConsoleCapabilities
{
    bool virtualTerminalProcessing = false;
    bool unicodeOutput = true;

    ConsoleColorTier colorTier = ConsoleColorTier::Basic16;

    ConsoleFeatureSupport bold = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport dim = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport underline = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport reverse = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport invisible = ConsoleFeatureSupport::Unknown;
    ConsoleFeatureSupport strike = ConsoleFeatureSupport::Unsupported;
    ConsoleFeatureSupport slowBlink = ConsoleFeatureSupport::Unsupported;
    ConsoleFeatureSupport fastBlink = ConsoleFeatureSupport::Unsupported;

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
};
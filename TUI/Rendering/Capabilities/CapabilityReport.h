#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Rendering/Capabilities/ConsoleCapabilities.h"
#include "Rendering/Styles/StylePolicy.h"

/*
    Purpose:

    CapabilityReport is a structured diagnostics model for renderer capability
    reporting and style adaptation reporting.

    It does not make rendering decisions.
    It only records:
        - detected backend capabilities
        - selected renderer adaptation policy
        - summary counts of adaptation outcomes
        - optional representative examples

    This model is intended to be optional and non-invasive.
*/

enum class StyleAdaptationKind
{
    Direct,
    Downgraded,
    Approximated,
    Omitted,
    Emulated,
    LogicalOnly
};

enum class StyleFeature
{
    ForegroundColor,
    BackgroundColor,
    Bold,
    Dim,
    Underline,
    SlowBlink,
    FastBlink,
    Reverse,
    Invisible,
    Strike
};

struct StyleAdaptationCounter
{
    StyleFeature feature = StyleFeature::ForegroundColor;
    StyleAdaptationKind kind = StyleAdaptationKind::Direct;
    std::size_t count = 0;
};

struct StyleAdaptationExample
{
    StyleFeature feature = StyleFeature::ForegroundColor;
    StyleAdaptationKind kind = StyleAdaptationKind::Direct;
    std::string detail;
};

struct BackendStateSnapshot
{
    std::string rendererIdentity = "Unknown";

    bool virtualTerminalEnableAttempted = false;
    bool virtualTerminalEnableSucceeded = false;

    std::uint32_t configuredOutputMode = 0;
    std::uint32_t configuredInputMode = 0;

    bool hasConfiguredOutputMode = false;
    bool hasConfiguredInputMode = false;
};

class CapabilityReport
{
public:
    CapabilityReport();

    void setCapabilities(const ConsoleCapabilities& capabilities);
    void setPolicy(const StylePolicy& policy);
    void setBackendState(const BackendStateSnapshot& backendState);

    const ConsoleCapabilities& capabilities() const;
    const StylePolicy& policy() const;
    const BackendStateSnapshot& backendState() const;

    void recordDirect(StyleFeature feature);
    void recordDowngraded(StyleFeature feature);
    void recordApproximated(StyleFeature feature);
    void recordOmitted(StyleFeature feature);
    void recordEmulated(StyleFeature feature);
    void recordLogicalOnly(StyleFeature feature);

    void addExample(
        StyleFeature feature,
        StyleAdaptationKind kind,
        const std::string& detail);

    std::size_t getCount(StyleFeature feature, StyleAdaptationKind kind) const;

    const std::vector<StyleAdaptationCounter>& counters() const;
    const std::vector<StyleAdaptationExample>& examples() const;

    void clearRuntimeData();
    bool hasRuntimeData() const;

    static const char* toString(ConsoleColorTier tier);
    static const char* toString(ConsoleFeatureSupport support);
    static const char* toString(ColorRenderMode mode);
    static const char* toString(TextAttributeRenderMode mode);
    static const char* toString(BlinkRenderMode mode);
    static const char* toString(StyleFeature feature);
    static const char* toString(StyleAdaptationKind kind);

private:
    void increment(StyleFeature feature, StyleAdaptationKind kind);

private:
    ConsoleCapabilities m_capabilities{};
    StylePolicy m_policy{};
    BackendStateSnapshot m_backendState{};

    std::vector<StyleAdaptationCounter> m_counters;
    std::vector<StyleAdaptationExample> m_examples;
};
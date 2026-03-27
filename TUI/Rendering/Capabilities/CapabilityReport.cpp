#include "Rendering/Capabilities/CapabilityReport.h"

CapabilityReport::CapabilityReport()
{
    const StyleFeature features[] =
    {
        StyleFeature::ForegroundColor,
        StyleFeature::BackgroundColor,
        StyleFeature::Bold,
        StyleFeature::Dim,
        StyleFeature::Underline,
        StyleFeature::SlowBlink,
        StyleFeature::FastBlink,
        StyleFeature::Reverse,
        StyleFeature::Invisible,
        StyleFeature::Strike
    };

    const StyleAdaptationKind kinds[] =
    {
        StyleAdaptationKind::Direct,
        StyleAdaptationKind::Downgraded,
        StyleAdaptationKind::Approximated,
        StyleAdaptationKind::Omitted,
        StyleAdaptationKind::Emulated,
        StyleAdaptationKind::LogicalOnly
    };

    for (StyleFeature feature : features)
    {
        for (StyleAdaptationKind kind : kinds)
        {
            StyleAdaptationCounter counter;
            counter.feature = feature;
            counter.kind = kind;
            counter.count = 0;
            m_counters.push_back(counter);
        }
    }
}

void CapabilityReport::setCapabilities(const ConsoleCapabilities& capabilities)
{
    m_capabilities = capabilities;
}

void CapabilityReport::setPolicy(const StylePolicy& policy)
{
    m_policy = policy;
}

void CapabilityReport::setBackendState(const BackendStateSnapshot& backendState)
{
    m_backendState = backendState;
}

const ConsoleCapabilities& CapabilityReport::capabilities() const
{
    return m_capabilities;
}

const StylePolicy& CapabilityReport::policy() const
{
    return m_policy;
}

const BackendStateSnapshot& CapabilityReport::backendState() const
{
    return m_backendState;
}

void CapabilityReport::recordDirect(StyleFeature feature)
{
    increment(feature, StyleAdaptationKind::Direct);
}

void CapabilityReport::recordDowngraded(StyleFeature feature)
{
    increment(feature, StyleAdaptationKind::Downgraded);
}

void CapabilityReport::recordApproximated(StyleFeature feature)
{
    increment(feature, StyleAdaptationKind::Approximated);
}

void CapabilityReport::recordOmitted(StyleFeature feature)
{
    increment(feature, StyleAdaptationKind::Omitted);
}

void CapabilityReport::recordEmulated(StyleFeature feature)
{
    increment(feature, StyleAdaptationKind::Emulated);
}

void CapabilityReport::recordLogicalOnly(StyleFeature feature)
{
    increment(feature, StyleAdaptationKind::LogicalOnly);
}

void CapabilityReport::addExample(
    StyleFeature feature,
    StyleAdaptationKind kind,
    const std::string& detail)
{
    StyleAdaptationExample example;
    example.feature = feature;
    example.kind = kind;
    example.detail = detail;
    m_examples.push_back(example);
}

std::size_t CapabilityReport::getCount(StyleFeature feature, StyleAdaptationKind kind) const
{
    for (const StyleAdaptationCounter& counter : m_counters)
    {
        if (counter.feature == feature && counter.kind == kind)
        {
            return counter.count;
        }
    }

    return 0;
}

const std::vector<StyleAdaptationCounter>& CapabilityReport::counters() const
{
    return m_counters;
}

const std::vector<StyleAdaptationExample>& CapabilityReport::examples() const
{
    return m_examples;
}

void CapabilityReport::clearRuntimeData()
{
    for (StyleAdaptationCounter& counter : m_counters)
    {
        counter.count = 0;
    }

    m_examples.clear();
}

bool CapabilityReport::hasRuntimeData() const
{
    for (const StyleAdaptationCounter& counter : m_counters)
    {
        if (counter.count > 0)
        {
            return true;
        }
    }

    return !m_examples.empty();
}

const char* CapabilityReport::toString(ConsoleColorTier tier)
{
    switch (tier)
    {
    case ConsoleColorTier::None:
        return "None";

    case ConsoleColorTier::Basic16:
        return "Basic16";

    case ConsoleColorTier::Indexed256:
        return "Indexed256";

    case ConsoleColorTier::TrueColor:
        return "TrueColor";

    default:
        return "Unknown";
    }
}

const char* CapabilityReport::toString(ConsoleFeatureSupport support)
{
    switch (support)
    {
    case ConsoleFeatureSupport::Unsupported:
        return "Unsupported";

    case ConsoleFeatureSupport::Supported:
        return "Supported";

    case ConsoleFeatureSupport::Emulated:
        return "Emulated";

    case ConsoleFeatureSupport::Unknown:
        return "Unknown";

    default:
        return "Unknown";
    }
}

const char* CapabilityReport::toString(ColorRenderMode mode)
{
    switch (mode)
    {
    case ColorRenderMode::Direct:
        return "Direct";

    case ColorRenderMode::DowngradeToBasic:
        return "DowngradeToBasic";

    case ColorRenderMode::DowngradeToIndexed256:
        return "DowngradeToIndexed256";

    case ColorRenderMode::Omit:
        return "Omit";

    default:
        return "Unknown";
    }
}

const char* CapabilityReport::toString(TextAttributeRenderMode mode)
{
    switch (mode)
    {
    case TextAttributeRenderMode::Direct:
        return "Direct";

    case TextAttributeRenderMode::Omit:
        return "Omit";

    default:
        return "Unknown";
    }
}

const char* CapabilityReport::toString(BlinkRenderMode mode)
{
    switch (mode)
    {
    case BlinkRenderMode::Direct:
        return "Direct";

    case BlinkRenderMode::Omit:
        return "Omit";

    case BlinkRenderMode::Emulate:
        return "Emulate";

    default:
        return "Unknown";
    }
}

const char* CapabilityReport::toString(StyleFeature feature)
{
    switch (feature)
    {
    case StyleFeature::ForegroundColor:
        return "ForegroundColor";

    case StyleFeature::BackgroundColor:
        return "BackgroundColor";

    case StyleFeature::Bold:
        return "Bold";

    case StyleFeature::Dim:
        return "Dim";

    case StyleFeature::Underline:
        return "Underline";

    case StyleFeature::SlowBlink:
        return "SlowBlink";

    case StyleFeature::FastBlink:
        return "FastBlink";

    case StyleFeature::Reverse:
        return "Reverse";

    case StyleFeature::Invisible:
        return "Invisible";

    case StyleFeature::Strike:
        return "Strike";

    default:
        return "Unknown";
    }
}

const char* CapabilityReport::toString(StyleAdaptationKind kind)
{
    switch (kind)
    {
    case StyleAdaptationKind::Direct:
        return "Direct";

    case StyleAdaptationKind::Downgraded:
        return "Downgraded";

    case StyleAdaptationKind::Approximated:
        return "Approximated";

    case StyleAdaptationKind::Omitted:
        return "Omitted";

    case StyleAdaptationKind::Emulated:
        return "Emulated";

    case StyleAdaptationKind::LogicalOnly:
        return "LogicalOnly";

    default:
        return "Unknown";
    }
}

void CapabilityReport::increment(StyleFeature feature, StyleAdaptationKind kind)
{
    for (StyleAdaptationCounter& counter : m_counters)
    {
        if (counter.feature == feature && counter.kind == kind)
        {
            ++counter.count;
            return;
        }
    }

    StyleAdaptationCounter counter;
    counter.feature = feature;
    counter.kind = kind;
    counter.count = 1;
    m_counters.push_back(counter);
}
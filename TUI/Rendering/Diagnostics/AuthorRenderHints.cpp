#include "Rendering/Diagnostics/AuthorRenderHints.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::size_t countOne(
        const CapabilityReport& report,
        StyleFeature feature,
        StyleAdaptationKind kind)
    {
        return report.getCount(feature, kind);
    }

    std::size_t countAny(
        const CapabilityReport& report,
        StyleFeature feature,
        std::initializer_list<StyleAdaptationKind> kinds)
    {
        std::size_t total = 0;

        for (StyleAdaptationKind kind : kinds)
        {
            total += report.getCount(feature, kind);
        }

        return total;
    }

    std::size_t countAcrossFeatures(
        const CapabilityReport& report,
        std::initializer_list<StyleFeature> features,
        std::initializer_list<StyleAdaptationKind> kinds)
    {
        std::size_t total = 0;

        for (StyleFeature feature : features)
        {
            total += countAny(report, feature, kinds);
        }

        return total;
    }

    bool hasAnyRuntimeEvidence(const CapabilityReport& report)
    {
        return report.hasRuntimeData();
    }

    std::string makeCountSentence(
        const char* prefix,
        std::size_t count,
        const char* singular,
        const char* plural)
    {
        std::ostringstream stream;
        stream
            << prefix
            << count
            << " "
            << (count == 1 ? singular : plural);
        return stream.str();
    }

    void addCoreModelExplanation(std::vector<std::string>& hints)
    {
        hints.push_back(
            "Authored logical style is the Style value requested by page/theme code. "
            "It remains preserved in ScreenBuffer even when the active backend cannot display every field directly.");

        hints.push_back(
            "Backend-supported capability describes what the active console path reports as directly supported or policy-eligible.");

        hints.push_back(
            "Actual rendered result is what became visible after runtime renderer adaptation such as direct mapping, downgrade, omission, approximation, or deferred emulation.");
    }

    void addRuntimeEvidenceOverview(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        if (!hasAnyRuntimeEvidence(report))
        {
            hints.push_back(
                "No runtime adaptation events were recorded during this diagnostics session, so author-facing hints are limited to the detected backend capability/policy context.");
            return;
        }

        const std::size_t nonDirectEvents = countAcrossFeatures(
            report,
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
            },
            {
                StyleAdaptationKind::Downgraded,
                StyleAdaptationKind::Approximated,
                StyleAdaptationKind::Omitted,
                StyleAdaptationKind::Emulated,
                StyleAdaptationKind::LogicalOnly
            });

            if (nonDirectEvents > 0)
            {
                hints.push_back(
                    makeCountSentence(
                        "Runtime diagnostics recorded ",
                        nonDirectEvents,
                        "non-direct adaptation event",
                        "non-direct adaptation events"));
            }
            else
            {
                hints.push_back(
                    "Runtime diagnostics recorded only direct mappings for the style features encountered during this session.");
            }
    }

    void addCapabilityTierHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const ConsoleCapabilities& capabilities = report.capabilities();

        if (capabilities.colorTier == ConsoleColorTier::Basic16)
        {
            hints.push_back(
                "The active backend is operating in Basic16 color mode. Rich authored colors may therefore be reduced before final presentation.");
        }
        else if (capabilities.colorTier == ConsoleColorTier::Indexed256)
        {
            hints.push_back(
                "The active backend supports Indexed256 color. Full TrueColor-authored colors may still be approximated or reduced to the nearest indexed entry.");
        }

        if (!capabilities.virtualTerminalProcessing)
        {
            hints.push_back(
                "Virtual terminal processing is not active for this backend path, so some richer terminal-style effects may be unavailable or handled conservatively.");
        }
    }

    void addColorHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const std::size_t downgraded =
            countOne(report, StyleFeature::ForegroundColor, StyleAdaptationKind::Downgraded) +
            countOne(report, StyleFeature::BackgroundColor, StyleAdaptationKind::Downgraded);

        const std::size_t approximated =
            countOne(report, StyleFeature::ForegroundColor, StyleAdaptationKind::Approximated) +
            countOne(report, StyleFeature::BackgroundColor, StyleAdaptationKind::Approximated);

        const std::size_t logicalOnly =
            countOne(report, StyleFeature::ForegroundColor, StyleAdaptationKind::LogicalOnly) +
            countOne(report, StyleFeature::BackgroundColor, StyleAdaptationKind::LogicalOnly);

        if (downgraded == 0 && approximated == 0 && logicalOnly == 0)
        {
            return;
        }

        std::ostringstream summary;
        summary
            << "Observed color adaptation during rendering: "
            << downgraded << " downgraded, "
            << approximated << " approximated, "
            << logicalOnly << " logical-only color outcome(s).";
        hints.push_back(summary.str());

        hints.push_back(
            "Portable authoring suggestion: prefer semantic theme colors built from standard or bright basic colors where stable cross-backend appearance matters.");

        if (logicalOnly > 0)
        {
            hints.push_back(
                "Some authored colors survived logically in the resolved style model but were not physically emitted by the current Win32 attribute path.");
        }
    }

    void addDecorationHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const std::size_t underlineOmitted =
            countOne(report, StyleFeature::Underline, StyleAdaptationKind::Omitted);
        const std::size_t underlineLogicalOnly =
            countOne(report, StyleFeature::Underline, StyleAdaptationKind::LogicalOnly);

        const std::size_t strikeOmitted =
            countOne(report, StyleFeature::Strike, StyleAdaptationKind::Omitted);
        const std::size_t strikeLogicalOnly =
            countOne(report, StyleFeature::Strike, StyleAdaptationKind::LogicalOnly);

        if ((underlineOmitted + underlineLogicalOnly + strikeOmitted + strikeLogicalOnly) == 0)
        {
            return;
        }

        if ((underlineOmitted + underlineLogicalOnly) > 0)
        {
            std::ostringstream stream;
            stream
                << "Underline requests encountered runtime adaptation: "
                << underlineOmitted << " omitted and "
                << underlineLogicalOnly << " logical-only case(s).";
            hints.push_back(stream.str());
        }

        if ((strikeOmitted + strikeLogicalOnly) > 0)
        {
            std::ostringstream stream;
            stream
                << "Strike requests encountered runtime adaptation: "
                << strikeOmitted << " omitted and "
                << strikeLogicalOnly << " logical-only case(s).";
            hints.push_back(stream.str());
        }

        hints.push_back(
            "Portable authoring suggestion: when underline or strike carries important meaning, reinforce it with wording, spacing, framing, icons, or contrast rather than relying on decoration alone.");
    }

    void addBlinkHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const std::size_t omitted =
            countOne(report, StyleFeature::SlowBlink, StyleAdaptationKind::Omitted) +
            countOne(report, StyleFeature::FastBlink, StyleAdaptationKind::Omitted);

        const std::size_t emulated =
            countOne(report, StyleFeature::SlowBlink, StyleAdaptationKind::Emulated) +
            countOne(report, StyleFeature::FastBlink, StyleAdaptationKind::Emulated);

        const std::size_t logicalOnly =
            countOne(report, StyleFeature::SlowBlink, StyleAdaptationKind::LogicalOnly) +
            countOne(report, StyleFeature::FastBlink, StyleAdaptationKind::LogicalOnly);

        if ((omitted + emulated + logicalOnly) == 0)
        {
            return;
        }

        std::ostringstream stream;
        stream
            << "Blink styling encountered runtime adaptation: "
            << omitted << " omitted, "
            << emulated << " emulated, "
            << logicalOnly << " logical-only case(s).";
        hints.push_back(stream.str());

        hints.push_back(
            "Portable authoring suggestion: do not rely on blink as the only signal for urgency. Pair it with stronger text, framing, reverse video, or high-contrast color.");
    }

    void addVisibilityHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const std::size_t reverseOmitted =
            countOne(report, StyleFeature::Reverse, StyleAdaptationKind::Omitted);
        const std::size_t reverseLogicalOnly =
            countOne(report, StyleFeature::Reverse, StyleAdaptationKind::LogicalOnly);

        const std::size_t invisibleOmitted =
            countOne(report, StyleFeature::Invisible, StyleAdaptationKind::Omitted);
        const std::size_t invisibleLogicalOnly =
            countOne(report, StyleFeature::Invisible, StyleAdaptationKind::LogicalOnly);

        if ((reverseOmitted + reverseLogicalOnly + invisibleOmitted + invisibleLogicalOnly) == 0)
        {
            return;
        }

        std::ostringstream stream;
        stream
            << "Visibility-related styling encountered runtime adaptation: reverse("
            << reverseOmitted << " omitted, "
            << reverseLogicalOnly << " logical-only), invisible("
            << invisibleOmitted << " omitted, "
            << invisibleLogicalOnly << " logical-only).";
        hints.push_back(stream.str());

        hints.push_back(
            "Portable authoring suggestion: when visibility semantics matter, use explicit text labels or layout cues instead of relying only on advanced style flags.");
    }

    void addExampleDrivenHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const std::vector<StyleAdaptationExample>& examples = report.examples();
        int added = 0;

        for (const StyleAdaptationExample& example : examples)
        {
            if (example.kind == StyleAdaptationKind::Direct)
            {
                continue;
            }

            std::string hint = "Observed during rendering: ";
            hint += example.detail;
            hints.push_back(hint);

            ++added;
            if (added >= 4)
            {
                break;
            }
        }
    }
}

std::vector<std::string> AuthorRenderHints::buildHints(const RenderDiagnostics& diagnostics)
{
    std::vector<std::string> hints;

    if (!diagnostics.isEnabled())
    {
        return hints;
    }

    const CapabilityReport& report = diagnostics.report();

    addCoreModelExplanation(hints);
    addRuntimeEvidenceOverview(report, hints);
    addCapabilityTierHints(report, hints);
    addColorHints(report, hints);
    addDecorationHints(report, hints);
    addBlinkHints(report, hints);
    addVisibilityHints(report, hints);
    addExampleDrivenHints(report, hints);

    return hints;
}
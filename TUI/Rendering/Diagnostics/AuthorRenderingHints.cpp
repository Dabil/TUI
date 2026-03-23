#include "Rendering/Diagnostics/AuthorRenderHints.h"

#include <string>
#include <vector>

namespace
{
    bool hasAnyCount(
        const CapabilityReport& report,
        StyleFeature feature,
        StyleAdaptationKind kind)
    {
        return report.getCount(feature, kind) > 0;
    }

    bool hasAnyColorDowngrade(const CapabilityReport& report)
    {
        return
            hasAnyCount(report, StyleFeature::ForegroundColor, StyleAdaptationKind::Downgraded) ||
            hasAnyCount(report, StyleFeature::ForegroundColor, StyleAdaptationKind::Approximated) ||
            hasAnyCount(report, StyleFeature::ForegroundColor, StyleAdaptationKind::LogicalOnly) ||
            hasAnyCount(report, StyleFeature::BackgroundColor, StyleAdaptationKind::Downgraded) ||
            hasAnyCount(report, StyleFeature::BackgroundColor, StyleAdaptationKind::Approximated) ||
            hasAnyCount(report, StyleFeature::BackgroundColor, StyleAdaptationKind::LogicalOnly);
    }

    bool hasAnyBlinkIssue(const CapabilityReport& report)
    {
        return
            hasAnyCount(report, StyleFeature::SlowBlink, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::SlowBlink, StyleAdaptationKind::Emulated) ||
            hasAnyCount(report, StyleFeature::SlowBlink, StyleAdaptationKind::LogicalOnly) ||
            hasAnyCount(report, StyleFeature::FastBlink, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::FastBlink, StyleAdaptationKind::Emulated) ||
            hasAnyCount(report, StyleFeature::FastBlink, StyleAdaptationKind::LogicalOnly);
    }

    bool hasAnyDecorationIssue(const CapabilityReport& report)
    {
        return
            hasAnyCount(report, StyleFeature::Underline, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::Underline, StyleAdaptationKind::LogicalOnly) ||
            hasAnyCount(report, StyleFeature::Strike, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::Strike, StyleAdaptationKind::LogicalOnly);
    }

    bool hasAnyVisibilityIssue(const CapabilityReport& report)
    {
        return
            hasAnyCount(report, StyleFeature::Invisible, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::Invisible, StyleAdaptationKind::LogicalOnly) ||
            hasAnyCount(report, StyleFeature::Reverse, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::Reverse, StyleAdaptationKind::LogicalOnly);
    }

    void addCoreModelExplanation(std::vector<std::string>& hints)
    {
        hints.push_back(
            "Authored logical style is the Style value requested by page/theme code. "
            "It is preserved in ScreenBuffer even when the active backend cannot display every field directly.");

        hints.push_back(
            "Backend-supported style is the subset of logical styling that the active console path reports as directly supported.");

        hints.push_back(
            "Actual rendered result is what becomes visible after renderer adaptation such as direct mapping, downgrade, omission, or deferred emulation.");
    }

    void addCapabilityTierHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const ConsoleCapabilities& capabilities = report.capabilities();

        if (capabilities.colorTier == ConsoleColorTier::Basic16)
        {
            hints.push_back(
                "This backend is currently operating in Basic16 color mode. "
                "Indexed256 and RGB authored colors may be reduced to nearest basic console colors.");
        }
        else if (capabilities.colorTier == ConsoleColorTier::Indexed256)
        {
            hints.push_back(
                "This backend supports Indexed256 color. Full RGB authored colors may still be approximated to the nearest indexed color.");
        }
        else if (capabilities.colorTier == ConsoleColorTier::TrueColor)
        {
            hints.push_back(
                "This backend reports TrueColor capability. Full RGB-authored styles are expected to map more faithfully than lower color tiers.");
        }

        if (!capabilities.virtualTerminalProcessing)
        {
            hints.push_back(
                "Virtual terminal processing is not active for this backend path. "
                "Some decorations and richer terminal features may therefore be unavailable or intentionally treated conservatively.");
        }
    }

    void addColorHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        if (!hasAnyColorDowngrade(report))
        {
            return;
        }

        hints.push_back(
            "One or more authored foreground/background colors were downgraded or approximated for the active backend.");

        hints.push_back(
            "Portable authoring suggestion: prefer semantic theme colors built from standard or bright basic colors for UI that must look stable across multiple console backends.");

        hints.push_back(
            "Use RGB or Indexed256 colors when they improve authored intent, but expect older or more limited backends to show a nearest available approximation.");
    }

    void addDecorationHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        if (!hasAnyDecorationIssue(report))
        {
            return;
        }

        if (hasAnyCount(report, StyleFeature::Underline, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::Underline, StyleAdaptationKind::LogicalOnly))
        {
            hints.push_back(
                "Underline was requested in authored style, but the active backend did not render all underline requests physically.");
        }

        if (hasAnyCount(report, StyleFeature::Strike, StyleAdaptationKind::Omitted) ||
            hasAnyCount(report, StyleFeature::Strike, StyleAdaptationKind::LogicalOnly))
        {
            hints.push_back(
                "Strike styling was requested in authored style, but the active backend did not render all strike requests physically.");
        }

        hints.push_back(
            "Portable authoring suggestion: when underline or strike carries important meaning, consider also using wording, spacing, framing, icons, or color contrast so the UI remains understandable on simpler backends.");
    }

    void addBlinkHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        if (!hasAnyBlinkIssue(report))
        {
            return;
        }

        hints.push_back(
            "Blink-related authored styles were encountered, but blink is commonly unsupported, omitted, or deferred to emulation on console backends.");

        hints.push_back(
            "Portable authoring suggestion: do not rely on blink as the only signal for urgency. Pair it with stronger text, frame treatment, reverse video, or high-contrast color when possible.");
    }

    void addVisibilityHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        if (!hasAnyVisibilityIssue(report))
        {
            return;
        }

        hints.push_back(
            "Some visibility-related style fields such as reverse or invisible were preserved logically but were not always rendered physically by the active backend.");

        hints.push_back(
            "Portable authoring suggestion: when visibility semantics matter, use explicit text labels or layout cues rather than relying only on advanced style flags.");
    }

    void addExampleDrivenHints(
        const CapabilityReport& report,
        std::vector<std::string>& hints)
    {
        const std::vector<StyleAdaptationExample>& examples = report.examples();
        int added = 0;

        for (const StyleAdaptationExample& example : examples)
        {
            if (example.kind == StyleAdaptationKind::Downgraded ||
                example.kind == StyleAdaptationKind::Approximated ||
                example.kind == StyleAdaptationKind::Omitted ||
                example.kind == StyleAdaptationKind::LogicalOnly ||
                example.kind == StyleAdaptationKind::Emulated)
            {
                std::string hint = "Observed example: ";
                hint += example.detail;
                hints.push_back(hint);

                ++added;
                if (added >= 3)
                {
                    break;
                }
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
    addCapabilityTierHints(report, hints);
    addColorHints(report, hints);
    addDecorationHints(report, hints);
    addBlinkHints(report, hints);
    addVisibilityHints(report, hints);
    addExampleDrivenHints(report, hints);

    if (hints.empty())
    {
        hints.push_back(
            "No author-facing rendering warnings were generated for this run. "
            "The authored logical styles either mapped directly or no adaptation events were recorded.");
    }

    return hints;
}
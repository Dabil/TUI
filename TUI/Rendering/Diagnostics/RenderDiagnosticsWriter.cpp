#include "Rendering/Diagnostics/RenderDiagnosticsWriter.h"

#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>

#include "Rendering/Diagnostics/AuthorRenderHints.h"

namespace
{
    void writeSectionHeader(std::ofstream& out, const char* title)
    {
        out << title << "\n";

        for (const char* p = title; *p != '\0'; ++p)
        {
            out << '=';
        }

        out << "\n\n";
    }

    std::string formatConsoleMode(std::uint32_t mode, bool hasValue)
    {
        if (!hasValue)
        {
            return "Unavailable";
        }

        std::ostringstream stream;
        stream
            << "0x"
            << std::uppercase
            << std::hex
            << std::setw(8)
            << std::setfill('0')
            << mode;

        return stream.str();
    }
}

bool RenderDiagnosticsWriter::write(const RenderDiagnostics& diagnostics)
{
    if (!diagnostics.isEnabled())
    {
        return true;
    }

    const std::ios::openmode mode =
        std::ios::out |
        (diagnostics.appendMode() ? std::ios::app : std::ios::trunc);

    std::ofstream out(diagnostics.outputPath(), mode);
    if (!out)
    {
        return false;
    }

    const CapabilityReport& report = diagnostics.report();
    const ConsoleCapabilities& capabilities = report.capabilities();
    const StylePolicy& policy = report.policy();
    const BackendStateSnapshot& backendState = report.backendState();

    writeSectionHeader(out, "Render Diagnostics Report");

    out << "Configuration\n";
    out << "-------------\n";
    out << "Output path: " << diagnostics.outputPath() << "\n";
    out << "Append mode: " << (diagnostics.appendMode() ? "true" : "false") << "\n";
    out << "Diagnostics enabled: " << (diagnostics.isEnabled() ? "true" : "false") << "\n\n";

    out << "Backend Configuration State\n";
    out << "---------------------------\n";
    out << "Renderer/backend identity: " << backendState.rendererIdentity << "\n";
    out << "VT enable attempted: "
        << (backendState.virtualTerminalEnableAttempted ? "true" : "false") << "\n";
    out << "VT enable succeeded: "
        << (backendState.virtualTerminalEnableSucceeded ? "true" : "false") << "\n";
    out << "Configured output mode: "
        << formatConsoleMode(backendState.configuredOutputMode, backendState.hasConfiguredOutputMode) << "\n";
    out << "Configured input mode: "
        << formatConsoleMode(backendState.configuredInputMode, backendState.hasConfiguredInputMode) << "\n\n";

    out << "Detected Backend Capabilities\n";
    out << "-----------------------------\n";
    out << "Virtual terminal processing: "
        << (capabilities.virtualTerminalProcessing ? "true" : "false") << "\n";
    out << "Unicode output: "
        << (capabilities.unicodeOutput ? "true" : "false") << "\n";
    out << "Color tier: "
        << CapabilityReport::toString(capabilities.colorTier) << "\n";
    out << "Bold: "
        << CapabilityReport::toString(capabilities.bold) << "\n";
    out << "Dim: "
        << CapabilityReport::toString(capabilities.dim) << "\n";
    out << "Underline: "
        << CapabilityReport::toString(capabilities.underline) << "\n";
    out << "Reverse: "
        << CapabilityReport::toString(capabilities.reverse) << "\n";
    out << "Invisible: "
        << CapabilityReport::toString(capabilities.invisible) << "\n";
    out << "Strike: "
        << CapabilityReport::toString(capabilities.strike) << "\n";
    out << "Slow blink: "
        << CapabilityReport::toString(capabilities.slowBlink) << "\n";
    out << "Fast blink: "
        << CapabilityReport::toString(capabilities.fastBlink) << "\n\n";

    out << "Resolved Renderer Adaptation Policy\n";
    out << "----------------------------------\n";
    out << "Basic colors: "
        << CapabilityReport::toString(policy.basicColorMode()) << "\n";
    out << "Indexed256 colors: "
        << CapabilityReport::toString(policy.indexed256ColorMode()) << "\n";
    out << "RGB colors: "
        << CapabilityReport::toString(policy.rgbColorMode()) << "\n";
    out << "Bold: "
        << CapabilityReport::toString(policy.boldMode()) << "\n";
    out << "Dim: "
        << CapabilityReport::toString(policy.dimMode()) << "\n";
    out << "Underline: "
        << CapabilityReport::toString(policy.underlineMode()) << "\n";
    out << "Reverse: "
        << CapabilityReport::toString(policy.reverseMode()) << "\n";
    out << "Invisible: "
        << CapabilityReport::toString(policy.invisibleMode()) << "\n";
    out << "Strike: "
        << CapabilityReport::toString(policy.strikeMode()) << "\n";
    out << "Slow blink: "
        << CapabilityReport::toString(policy.slowBlinkMode()) << "\n";
    out << "Fast blink: "
        << CapabilityReport::toString(policy.fastBlinkMode()) << "\n\n";

    out << "Adaptation Summary Counts\n";
    out << "-------------------------\n";

    for (const StyleAdaptationCounter& counter : report.counters())
    {
        if (counter.count == 0)
        {
            continue;
        }

        out
            << CapabilityReport::toString(counter.feature)
            << " / "
            << CapabilityReport::toString(counter.kind)
            << ": "
            << counter.count
            << "\n";
    }

    if (!report.hasRuntimeData())
    {
        out << "No runtime adaptation events were recorded.\n";
    }

    out << "\n";

    out << "Representative Examples\n";
    out << "-----------------------\n";

    if (report.examples().empty())
    {
        out << "No examples recorded.\n";
    }
    else
    {
        for (const StyleAdaptationExample& example : report.examples())
        {
            out
                << "["
                << CapabilityReport::toString(example.feature)
                << " / "
                << CapabilityReport::toString(example.kind)
                << "] "
                << example.detail
                << "\n";
        }
    }

    out << "\n";

    out << "Author-Facing Rendering Hints\n";
    out << "----------------------------\n";

    const std::vector<std::string> hints = AuthorRenderHints::buildHints(diagnostics);
    for (const std::string& hint : hints)
    {
        out << "- " << hint << "\n";
    }

    out << "\n";

    out << "Notes\n";
    out << "-----\n";
    out << "- Diagnostics describe renderer behavior only.\n";
    out << "- Logical Style data stored in ScreenBuffer is not mutated by diagnostics or adaptation.\n";
    out << "- Output differences between authored style and visible result may be caused by downgrade, omission, approximation, or deferred emulation.\n";
    out << "- Author-facing hints are advisory only and do not change rendering behavior.\n";

    return true;
}
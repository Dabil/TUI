#include "Rendering/Composition/WritePolicy.h"

namespace Composition
{
    WritePolicy WritePolicy::ReplaceAll()
    {
        WritePolicy policy;
        policy.glyphPolicy = GlyphPolicy::All;
        policy.stylePolicy = StylePolicy::Apply;
        policy.sourceMask = SourceMask::AllCells;
        policy.glyphOverwriteRule = OverwriteRule::Always;
        policy.styleOverwriteRule = OverwriteRule::Always;
        policy.depthPolicy = DepthPolicy::Ignore;
        return policy;
    }

    WritePolicy WritePolicy::TransparentGlyphOverlay()
    {
        WritePolicy policy;
        policy.glyphPolicy = GlyphPolicy::NonSpaceOnly;
        policy.stylePolicy = StylePolicy::Apply;
        policy.sourceMask = SourceMask::GlyphCellsOnly;
        policy.glyphOverwriteRule = OverwriteRule::Always;
        policy.styleOverwriteRule = OverwriteRule::Always;
        policy.depthPolicy = DepthPolicy::Ignore;
        return policy;
    }

    WritePolicy WritePolicy::StyleOnly()
    {
        WritePolicy policy;
        policy.glyphPolicy = GlyphPolicy::None;
        policy.stylePolicy = StylePolicy::Apply;
        policy.sourceMask = SourceMask::AllCells;
        policy.glyphOverwriteRule = OverwriteRule::Never;
        policy.styleOverwriteRule = OverwriteRule::Always;
        policy.depthPolicy = DepthPolicy::Ignore;
        return policy;
    }

    WritePolicy WritePolicy::NoWrite()
    {
        WritePolicy policy;
        policy.glyphPolicy = GlyphPolicy::None;
        policy.stylePolicy = StylePolicy::None;
        policy.sourceMask = SourceMask::AllCells;
        policy.glyphOverwriteRule = OverwriteRule::Never;
        policy.styleOverwriteRule = OverwriteRule::Never;
        policy.depthPolicy = DepthPolicy::Ignore;
        return policy;
    }

    bool WritePolicy::writesGlyphs() const
    {
        return glyphPolicy != GlyphPolicy::None &&
            glyphOverwriteRule != OverwriteRule::Never;
    }

    bool WritePolicy::writesStyles() const
    {
        return stylePolicy == StylePolicy::Apply &&
            styleOverwriteRule != OverwriteRule::Never;
    }

    bool WritePolicy::canWriteAnything() const
    {
        return writesGlyphs() || writesStyles();
    }

    bool WritePolicy::operator==(const WritePolicy& other) const
    {
        return glyphPolicy == other.glyphPolicy &&
            stylePolicy == other.stylePolicy &&
            sourceMask == other.sourceMask &&
            glyphOverwriteRule == other.glyphOverwriteRule &&
            styleOverwriteRule == other.styleOverwriteRule &&
            depthPolicy == other.depthPolicy;
    }

    bool WritePolicy::operator!=(const WritePolicy& other) const
    {
        return !(*this == other);
    }
}
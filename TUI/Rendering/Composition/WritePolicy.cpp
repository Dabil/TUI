#include "Rendering/Composition/WritePolicy.h"

#include "Rendering/Composition/WritePresets.h"

/*
    Concrete Rules:
    
    TextObject cell coordinates are absolute within the TextObject.

    WritePolicy may decide:
        - whether a source cell mutates the destination cell
        - whether glyph/style/depth writes are allowed

    WritePolicy must never decide:
        - the source coordinate of later cells
        - cursor advancement
        - measured width
        - object bounds
*/

namespace Composition
{
    WritePolicy WritePolicy::VisibleObject()
    {
        return WritePresets::visibleObject();
    }

    WritePolicy WritePolicy::AuthoredObject()
    {
        return WritePresets::authoredObject();
    }

    WritePolicy WritePolicy::SolidObject()
    {
        return WritePresets::solidObject();
    }

    WritePolicy WritePolicy::StyleOnly()
    {
        return WritePresets::styleBlock();
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
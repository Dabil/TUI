#pragma once

#include "Rendering/Composition/DepthPolicy.h"
#include "Rendering/Composition/GlyphPolicy.h"
#include "Rendering/Composition/OverwriteRule.h"
#include "Rendering/Composition/SourceMask.h"
#include "Rendering/Composition/StylePolicy.h"

namespace Composition
{
    /*
        WritePolicy is the core engine-facing composition policy object.

        It fully describes how a write pass should behave without relying on:
        - hidden mutable writer state
        - ambiguous legacy mode flags
        - backend-specific rendering assumptions

        The intent is that future semantic wrappers translate into one of these
        explicit policies before any actual composition work is performed.
    */
    struct WritePolicy
    {
        GlyphPolicy glyphPolicy = GlyphPolicy::All;
        StylePolicy stylePolicy = StylePolicy::Apply;
        SourceMask sourceMask = SourceMask::AllCells;

        OverwriteRule glyphOverwriteRule = OverwriteRule::Always;
        OverwriteRule styleOverwriteRule = OverwriteRule::Always;

        DepthPolicy depthPolicy = DepthPolicy::Ignore;

        static WritePolicy ReplaceAll();
        static WritePolicy TransparentGlyphOverlay();
        static WritePolicy StyleOnly();
        static WritePolicy NoWrite();

        bool writesGlyphs() const;
        bool writesStyles() const;
        bool canWriteAnything() const;

        bool operator==(const WritePolicy& other) const;
        bool operator!=(const WritePolicy& other) const;
    };
}
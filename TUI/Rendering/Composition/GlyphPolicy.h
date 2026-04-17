#pragma once

namespace Composition
{
    /*
        GlyphPolicy answers:

            "If a participating source cell is considered, what glyph-writing
             intent should the engine apply?"

        Semantics:
        - None:
            Never write glyph content from the source.
        - NonSpaceOnly:
            Write source glyphs only when the source cell represents a non-space glyph.
            Source spaces do not replace destination glyphs.
        - All:
            Write all eligible source glyphs, including spaces.
    */
    enum class GlyphPolicy
    {
        None,
        NonSpaceOnly,
        All
    };
}
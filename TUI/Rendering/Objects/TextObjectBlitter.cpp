#include "Rendering/Composition/ObjectWriter.h"
#include "Rendering/Objects/TextObjectBlitter.h"

namespace
{
    bool shouldSkipCell(
        const TextObjectCell& cell,
        const TextObjectBlitter::BlitOptions& options)
    {
        if (options.skipEmptyCells && cell.kind == CellKind::Empty)
        {
            return true;
        }

        if (options.skipStructuralContinuationCells)
        {
            if (cell.kind == CellKind::WideTrailing)
            {
                return true;
            }

            if (cell.kind == CellKind::CombiningContinuation)
            {
                return true;
            }
        }

        return false;
    }

    std::optional<Style> resolveBuilderStyle(
        const TextObjectCell& sourceCell,
        const TextObjectBlitter::BlitOptions& options)
    {
        if (options.overrideStyle.has_value())
        {
            return options.overrideStyle;
        }

        return sourceCell.style;
    }
}

namespace TextObjectBlitter
{
    void blitToBuilder(
        TextObjectBuilder& builder,
        const TextObject& source,
        int offsetX,
        int offsetY,
        const BlitOptions& options)
    {
        if (!source.isLoaded() || source.getWidth() <= 0 || source.getHeight() <= 0)
        {
            return;
        }

        for (int y = 0; y < source.getHeight(); ++y)
        {
            for (int x = 0; x < source.getWidth(); ++x)
            {
                const TextObjectCell* cell = source.tryGetCell(x, y);
                if (cell == nullptr || shouldSkipCell(*cell, options))
                {
                    continue;
                }

                const int destX = offsetX + x;
                const int destY = offsetY + y;
                if (!builder.inBounds(destX, destY))
                {
                    continue;
                }

                const std::optional<Style> styleToApply = resolveBuilderStyle(*cell, options);

                switch (cell->kind)
                {
                case CellKind::Glyph:
                    if (cell->width == CellWidth::Two)
                    {
                        builder.setWideGlyph(destX, destY, cell->glyph, styleToApply);
                    }
                    else
                    {
                        builder.setGlyph(destX, destY, cell->glyph, styleToApply);
                    }
                    break;

                case CellKind::WideTrailing:
                case CellKind::CombiningContinuation:
                    builder.setCell(destX, destY, cell->glyph, cell->kind, cell->width, styleToApply);
                    break;

                case CellKind::Empty:
                default:
                    break;
                }
            }
        }
    }

    void blitToScreenBuffer(
        ScreenBuffer& target,
        const TextObject& source,
        int offsetX,
        int offsetY,
        const BlitOptions& options)
    {
        Composition::WritePolicy policy;
        policy.glyphPolicy = Composition::GlyphPolicy::All;
        policy.stylePolicy = Composition::StylePolicy::Apply;
        policy.sourceMask = options.skipEmptyCells
            ? Composition::SourceMask::GlyphCellsOnly
            : Composition::SourceMask::AllCells;
        policy.glyphOverwriteRule = Composition::OverwriteRule::Always;
        policy.styleOverwriteRule = Composition::OverwriteRule::Always;
        policy.depthPolicy = Composition::DepthPolicy::Ignore;

        Composition::ObjectWriter writer(target, offsetX, offsetY);
        writer.writeObject(source, policy, options.overrideStyle);
    }
}
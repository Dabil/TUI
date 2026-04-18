#include "Rendering/Composition/ObjectWriter.h"

#include <array>

#include "Rendering/Composition/WritePresets.h"

namespace
{
    struct LogicalCellRef
    {
        int x = 0;
        int y = 0;
        const ScreenCell* cell = nullptr;
    };

    bool isSourceLeadingCell(const TextObjectCell& cell)
    {
        return cell.kind == CellKind::Glyph || cell.kind == CellKind::Empty;
    }

    bool sourceMaskAllows(const TextObjectCell& cell, Composition::SourceMask mask)
    {
        switch (mask)
        {
        case Composition::SourceMask::GlyphCellsOnly:
            return cell.kind == CellKind::Glyph;

        case Composition::SourceMask::SpaceCellsOnly:
            return cell.kind == CellKind::Empty;

        case Composition::SourceMask::AllCells:
        default:
            return cell.kind == CellKind::Glyph || cell.kind == CellKind::Empty;
        }
    }

    bool glyphPolicyAllows(const TextObjectCell& cell, Composition::GlyphPolicy policy)
    {
        switch (policy)
        {
        case Composition::GlyphPolicy::None:
            return false;

        case Composition::GlyphPolicy::NonSpaceOnly:
            return cell.kind == CellKind::Glyph && cell.glyph != U' ';

        case Composition::GlyphPolicy::All:
        default:
            return cell.kind == CellKind::Glyph || cell.kind == CellKind::Empty;
        }
    }

    const ScreenCell& resolveLogicalDestinationCell(const ScreenBuffer& target, int x, int y)
    {
        const ScreenCell& cell = target.getCell(x, y);

        if (cell.kind == CellKind::WideTrailing && target.inBounds(x - 1, y))
        {
            const ScreenCell& leading = target.getCell(x - 1, y);
            if (leading.kind == CellKind::Glyph && leading.width == CellWidth::Two)
            {
                return leading;
            }
        }

        return cell;
    }

    Style resolveLogicalDestinationStyle(const ScreenBuffer& target, int x, int y)
    {
        return resolveLogicalDestinationCell(target, x, y).style;
    }

    int collectLogicalDestinationCells(
        const ScreenBuffer& target,
        int x,
        int y,
        int width,
        std::array<LogicalCellRef, 2>& outCells)
    {
        int count = 0;

        for (int dx = 0; dx < width; ++dx)
        {
            const int currentX = x + dx;
            const ScreenCell& logicalCell = resolveLogicalDestinationCell(target, currentX, y);

            int logicalX = currentX;
            if (target.getCell(currentX, y).kind == CellKind::WideTrailing)
            {
                logicalX = currentX - 1;
            }

            bool alreadyAdded = false;
            for (int i = 0; i < count; ++i)
            {
                if (outCells[i].x == logicalX && outCells[i].y == y)
                {
                    alreadyAdded = true;
                    break;
                }
            }

            if (!alreadyAdded)
            {
                outCells[count].x = logicalX;
                outCells[count].y = y;
                outCells[count].cell = &logicalCell;
                ++count;
            }
        }

        return count;
    }

    bool overwriteRuleAllows(
        const ScreenBuffer& target,
        int x,
        int y,
        int width,
        Composition::OverwriteRule rule)
    {
        switch (rule)
        {
        case Composition::OverwriteRule::Never:
            return false;

        case Composition::OverwriteRule::Always:
            return true;

        case Composition::OverwriteRule::IfTargetEmpty:
        case Composition::OverwriteRule::IfTargetNonEmpty:
            break;
        }

        std::array<LogicalCellRef, 2> logicalCells{};
        const int logicalCellCount = collectLogicalDestinationCells(target, x, y, width, logicalCells);

        for (int i = 0; i < logicalCellCount; ++i)
        {
            const bool empty =
                logicalCells[i].cell == nullptr ||
                logicalCells[i].cell->kind == CellKind::Empty;

            if (rule == Composition::OverwriteRule::IfTargetEmpty && !empty)
            {
                return false;
            }

            if (rule == Composition::OverwriteRule::IfTargetNonEmpty && empty)
            {
                return false;
            }
        }

        return true;
    }

    bool depthPolicyAllows(
        const ScreenBuffer& target,
        int x,
        int y,
        int width,
        Composition::DepthPolicy policy)
    {
        (void)target;
        (void)x;
        (void)y;
        (void)width;

        switch (policy)
        {
        case Composition::DepthPolicy::Ignore:
        case Composition::DepthPolicy::Respect:
            return true;

        case Composition::DepthPolicy::BehindOnly:
            // Explicit extension seam for future true depth ownership.
            // Phase 4 does not yet have source-side depth data, so we avoid
            // inventing fake comparisons that would become a hidden policy trap.
            return false;

        default:
            return true;
        }
    }

    bool isValidSourceWideGlyph(const TextObject& source, int x, int y)
    {
        if (x + 1 >= source.getWidth())
        {
            return false;
        }

        const TextObjectCell* trailing = source.tryGetCell(x + 1, y);
        if (trailing == nullptr)
        {
            return false;
        }

        return trailing->kind == CellKind::WideTrailing;
    }

    std::optional<Style> resolveSourceStyle(
        const TextObjectCell& sourceCell,
        const std::optional<Style>& sourceStyleOverride)
    {
        if (sourceStyleOverride.has_value())
        {
            return sourceStyleOverride;
        }

        return sourceCell.style;
    }

    void writeWithPreset(
        Composition::ObjectWriter& writer,
        const TextObject& source,
        const std::optional<Style>& sourceStyleOverride,
        const Composition::WritePolicy& policy)
    {
        writer.writeObject(source, policy, sourceStyleOverride);
    }
}

namespace Composition
{
    ObjectWriter::ObjectWriter(ScreenBuffer& target, int offsetX, int offsetY)
        : m_target(target)
        , m_offsetX(offsetX)
        , m_offsetY(offsetY)
    {
    }

    void ObjectWriter::setOffset(int offsetX, int offsetY)
    {
        m_offsetX = offsetX;
        m_offsetY = offsetY;
    }

    int ObjectWriter::offsetX() const
    {
        return m_offsetX;
    }

    int ObjectWriter::offsetY() const
    {
        return m_offsetY;
    }

    void ObjectWriter::writeObject(const TextObject& source, const WritePolicy& policy)
    {
        writeObjectInternal(source, policy, std::nullopt);
    }

    void ObjectWriter::writeObject(
        const TextObject& source,
        const WritePolicy& policy,
        const std::optional<Style>& sourceStyleOverride)
    {
        writeObjectInternal(source, policy, sourceStyleOverride);
    }

    void ObjectWriter::writeObjectInternal(
        const TextObject& source,
        const WritePolicy& policy,
        const std::optional<Style>& sourceStyleOverride)
    {
        if (!source.isLoaded() || source.getWidth() <= 0 || source.getHeight() <= 0)
        {
            return;
        }

        if (!policy.canWriteAnything())
        {
            return;
        }

        for (int sourceY = 0; sourceY < source.getHeight(); ++sourceY)
        {
            for (int sourceX = 0; sourceX < source.getWidth(); ++sourceX)
            {
                const TextObjectCell* sourceCell = source.tryGetCell(sourceX, sourceY);
                if (sourceCell == nullptr || !isSourceLeadingCell(*sourceCell))
                {
                    continue;
                }

                if (!sourceMaskAllows(*sourceCell, policy.sourceMask))
                {
                    continue;
                }

                const int destinationX = m_offsetX + sourceX;
                const int destinationY = m_offsetY + sourceY;

                const int writeWidth = sourceCell->width == CellWidth::Two ? 2 : 1;
                if (!m_target.inBounds(destinationX, destinationY) ||
                    !m_target.inBounds(destinationX + writeWidth - 1, destinationY))
                {
                    continue;
                }

                if (sourceCell->width == CellWidth::Two &&
                    !isValidSourceWideGlyph(source, sourceX, sourceY))
                {
                    continue;
                }

                if (!depthPolicyAllows(m_target, destinationX, destinationY, writeWidth, policy.depthPolicy))
                {
                    continue;
                }

                const bool canWriteGlyph =
                    glyphPolicyAllows(*sourceCell, policy.glyphPolicy) &&
                    overwriteRuleAllows(
                        m_target,
                        destinationX,
                        destinationY,
                        writeWidth,
                        policy.glyphOverwriteRule);

                const std::optional<Style> resolvedSourceStyle =
                    resolveSourceStyle(*sourceCell, sourceStyleOverride);

                const bool canWriteStyle =
                    policy.stylePolicy == Composition::StylePolicy::Apply &&
                    resolvedSourceStyle.has_value() &&
                    overwriteRuleAllows(
                        m_target,
                        destinationX,
                        destinationY,
                        writeWidth,
                        policy.styleOverwriteRule);

                if (!canWriteGlyph && !canWriteStyle)
                {
                    continue;
                }

                if (canWriteGlyph)
                {
                    ScreenCell destinationCell;
                    destinationCell.glyph = sourceCell->glyph;
                    destinationCell.kind = sourceCell->kind;
                    destinationCell.width = sourceCell->width;
                    destinationCell.style = canWriteStyle
                        ? *resolvedSourceStyle
                        : resolveLogicalDestinationStyle(m_target, destinationX, destinationY);

                    m_target.setCell(destinationX, destinationY, destinationCell);
                    continue;
                }

                m_target.setCellStyle(destinationX, destinationY, *resolvedSourceStyle);
            }
        }
    }

    void ObjectWriter::writeSolidObject(
        const TextObject& source,
        DepthPolicy depthPolicy)
    {
        writeSolidObject(source, std::nullopt, depthPolicy);
    }

    void ObjectWriter::writeSolidObject(
        const TextObject& source,
        const Style& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeSolidObject(source, std::optional<Style>(sourceStyleOverride), depthPolicy);
    }

    void ObjectWriter::writeSolidObject(
        const TextObject& source,
        const std::optional<Style>& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeWithPreset(*this, source, sourceStyleOverride, WritePresets::solidObject(depthPolicy));
    }

    void ObjectWriter::writeVisibleObject(
        const TextObject& source,
        DepthPolicy depthPolicy)
    {
        writeVisibleObject(source, std::nullopt, depthPolicy);
    }

    void ObjectWriter::writeVisibleObject(
        const TextObject& source,
        const Style& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeVisibleObject(source, std::optional<Style>(sourceStyleOverride), depthPolicy);
    }

    void ObjectWriter::writeVisibleObject(
        const TextObject& source,
        const std::optional<Style>& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeWithPreset(*this, source, sourceStyleOverride, WritePresets::visibleObject(depthPolicy));
    }

    void ObjectWriter::writeGlyphsOnly(
        const TextObject& source,
        DepthPolicy depthPolicy)
    {
        writeGlyphsOnly(source, std::nullopt, depthPolicy);
    }

    void ObjectWriter::writeGlyphsOnly(
        const TextObject& source,
        const Style& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeGlyphsOnly(source, std::optional<Style>(sourceStyleOverride), depthPolicy);
    }

    void ObjectWriter::writeGlyphsOnly(
        const TextObject& source,
        const std::optional<Style>& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeWithPreset(*this, source, sourceStyleOverride, WritePresets::glyphsOnly(depthPolicy));
    }

    void ObjectWriter::writeStyleMask(
        const TextObject& source,
        DepthPolicy depthPolicy)
    {
        writeStyleMask(source, std::nullopt, depthPolicy);
    }

    void ObjectWriter::writeStyleMask(
        const TextObject& source,
        const Style& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeStyleMask(source, std::optional<Style>(sourceStyleOverride), depthPolicy);
    }

    void ObjectWriter::writeStyleMask(
        const TextObject& source,
        const std::optional<Style>& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeWithPreset(*this, source, sourceStyleOverride, WritePresets::styleMask(depthPolicy));
    }

    void ObjectWriter::writeStyleBlock(
        const TextObject& source,
        DepthPolicy depthPolicy)
    {
        writeStyleBlock(source, std::nullopt, depthPolicy);
    }

    void ObjectWriter::writeStyleBlock(
        const TextObject& source,
        const Style& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeStyleBlock(source, std::optional<Style>(sourceStyleOverride), depthPolicy);
    }

    void ObjectWriter::writeStyleBlock(
        const TextObject& source,
        const std::optional<Style>& sourceStyleOverride,
        DepthPolicy depthPolicy)
    {
        writeWithPreset(*this, source, sourceStyleOverride, WritePresets::styleBlock(depthPolicy));
    }
}
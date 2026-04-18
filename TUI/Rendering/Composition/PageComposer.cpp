#include "Rendering/Composition/PageComposer.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Utilities/Unicode/GraphemeSegmentation.h"
#include "Utilities/Unicode/UnicodeConversion.h"

namespace
{
    bool isLineBreak(char32_t ch)
    {
        return ch == U'\n' || ch == U'\r';
    }
}

namespace Composition
{
    PageComposer::PageComposer(ScreenBuffer& target)
        : m_target(&target)
        , m_composedBuffer(target)
    {
    }

    void PageComposer::resize(int width, int height)
    {
        m_composedBuffer.resize(width, height);
        synchronizeTarget();
    }

    int PageComposer::getWidth() const
    {
        return m_composedBuffer.getWidth();
    }

    int PageComposer::getHeight() const
    {
        return m_composedBuffer.getHeight();
    }

    void PageComposer::setTarget(ScreenBuffer& target)
    {
        m_target = &target;

        if (m_composedBuffer.getWidth() <= 0 || m_composedBuffer.getHeight() <= 0)
        {
            m_composedBuffer = target;
            return;
        }

        synchronizeTarget();
    }

    void PageComposer::detachTarget()
    {
        m_target = nullptr;
    }

    bool PageComposer::hasTarget() const
    {
        return m_target != nullptr;
    }

    ScreenBuffer* PageComposer::tryGetTarget()
    {
        return m_target;
    }

    const ScreenBuffer* PageComposer::tryGetTarget() const
    {
        return m_target;
    }

    ScreenBuffer& PageComposer::getTarget()
    {
        if (m_target == nullptr)
        {
            throw std::runtime_error("PageComposer has no target ScreenBuffer.");
        }

        return *m_target;
    }

    const ScreenBuffer& PageComposer::getTarget() const
    {
        if (m_target == nullptr)
        {
            throw std::runtime_error("PageComposer has no target ScreenBuffer.");
        }

        return *m_target;
    }

    ScreenBuffer PageComposer::captureBuffer() const
    {
        return m_composedBuffer;
    }

    std::u32string PageComposer::renderToU32String() const
    {
        return m_composedBuffer.renderToU32String();
    }

    std::string PageComposer::renderToUtf8String() const
    {
        return m_composedBuffer.renderToUtf8String();
    }

    void PageComposer::clear(const Style& style)
    {
        m_composedBuffer.clear(style);
        synchronizeTarget();
    }

    void PageComposer::setScreenTemplateLoader(ScreenTemplateLoader loader)
    {
        m_screenTemplateLoader = std::move(loader);
    }

    bool PageComposer::hasScreenTemplateLoader() const
    {
        return static_cast<bool>(m_screenTemplateLoader);
    }

    bool PageComposer::createScreen(std::string_view filename)
    {
        if (!m_screenTemplateLoader)
        {
            return false;
        }

        const ScreenTemplateLoadResult loadResult = m_screenTemplateLoader(filename);
        if (!loadResult.success)
        {
            return false;
        }

        return loadScreen(loadResult.object);
    }

    bool PageComposer::loadScreen(const TextObject& object)
    {
        return loadScreen(object, WritePresets::solidObject(), std::nullopt);
    }

    bool PageComposer::loadScreen(
        const TextObject& object,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        if (!object.isLoaded())
        {
            return false;
        }

        object.draw(m_composedBuffer, 0, 0, writePolicy, overrideStyle);
        synchronizeTarget();
        return true;
    }

    bool PageComposer::createRegion(
        int x,
        int y,
        int width,
        int height,
        std::string_view name)
    {
        return m_regions.createRegion(x, y, width, height, name);
    }

    bool PageComposer::hasRegion(std::string_view name) const
    {
        return m_regions.hasRegion(name);
    }

    const NamedRegion* PageComposer::getRegion(std::string_view name) const
    {
        return m_regions.getRegion(name);
    }

    NamedRegion* PageComposer::getRegion(std::string_view name)
    {
        return m_regions.getRegion(name);
    }

    void PageComposer::clearRegions()
    {
        m_regions.clearRegions();
    }

    Rect PageComposer::getFullScreenRegion() const
    {
        return makeRect(0, 0, std::max(0, getWidth()), std::max(0, getHeight()));
    }

    Point PageComposer::writeObject(
        const TextObject& object,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return drawObjectAt(object, x, y, writePolicy, overrideStyle);
    }

    Point PageComposer::writeSolidObject(
        const TextObject& object,
        int x,
        int y,
        const std::optional<Style>& overrideStyle)
    {
        return writeObject(object, x, y, WritePresets::solidObject(), overrideStyle);
    }

    Point PageComposer::writeVisibleObject(
        const TextObject& object,
        int x,
        int y,
        const std::optional<Style>& overrideStyle)
    {
        return writeObject(object, x, y, WritePresets::visibleObject(), overrideStyle);
    }

    Point PageComposer::writeGlyphsOnly(
        const TextObject& object,
        int x,
        int y,
        const std::optional<Style>& overrideStyle)
    {
        return writeObject(object, x, y, WritePresets::glyphsOnly(), overrideStyle);
    }

    Point PageComposer::writeStyleMask(
        const TextObject& object,
        int x,
        int y,
        const std::optional<Style>& overrideStyle)
    {
        return writeObject(object, x, y, WritePresets::styleMask(), overrideStyle);
    }

    Point PageComposer::writeStyleBlock(
        const TextObject& object,
        int x,
        int y,
        const std::optional<Style>& overrideStyle)
    {
        return writeObject(object, x, y, WritePresets::styleBlock(), overrideStyle);
    }

    PlacementResult PageComposer::writeObject(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        PlacementRequest request;
        request.region = region;
        request.contentSize = measureObject(object);
        request.alignment = alignment;
        request.clampToRegion = clampToRegion;

        PlacementResult result = resolvePlacement(request);
        drawObjectAt(
            object,
            result.origin.x,
            result.origin.y,
            writePolicy,
            overrideStyle);

        return result;
    }

    PlacementResult PageComposer::writeSolidObject(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObject(
            object,
            region,
            alignment,
            WritePresets::solidObject(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeVisibleObject(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObject(
            object,
            region,
            alignment,
            WritePresets::visibleObject(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeGlyphsOnly(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObject(
            object,
            region,
            alignment,
            WritePresets::glyphsOnly(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeStyleMask(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObject(
            object,
            region,
            alignment,
            WritePresets::styleMask(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeStyleBlock(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObject(
            object,
            region,
            alignment,
            WritePresets::styleBlock(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeObjectInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        const NamedRegion* region = m_regions.getRegion(regionName);
        if (region == nullptr)
        {
            return makeUnresolvedPlacementResult(object, alignment);
        }

        return writeObject(
            object,
            region->bounds,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeSolidObjectInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectInRegion(
            object,
            regionName,
            alignment,
            WritePresets::solidObject(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeVisibleObjectInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectInRegion(
            object,
            regionName,
            alignment,
            WritePresets::visibleObject(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeGlyphsOnlyInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectInRegion(
            object,
            regionName,
            alignment,
            WritePresets::glyphsOnly(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeStyleMaskInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectInRegion(
            object,
            regionName,
            alignment,
            WritePresets::styleMask(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeStyleBlockInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectInRegion(
            object,
            regionName,
            alignment,
            WritePresets::styleBlock(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeObjectAligned(
        const TextObject& object,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObject(
            object,
            getFullScreenRegion(),
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeSolidObjectAligned(
        const TextObject& object,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectAligned(
            object,
            alignment,
            WritePresets::solidObject(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeVisibleObjectAligned(
        const TextObject& object,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectAligned(
            object,
            alignment,
            WritePresets::visibleObject(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeGlyphsOnlyAligned(
        const TextObject& object,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectAligned(
            object,
            alignment,
            WritePresets::glyphsOnly(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeStyleMaskAligned(
        const TextObject& object,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectAligned(
            object,
            alignment,
            WritePresets::styleMask(),
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::writeStyleBlockAligned(
        const TextObject& object,
        const Alignment& alignment,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectAligned(
            object,
            alignment,
            WritePresets::styleBlock(),
            overrideStyle,
            clampToRegion);
    }

    Point PageComposer::placeObject(
        const TextObject& object,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return writeObject(object, x, y, writePolicy, overrideStyle);
    }

    PlacementResult PageComposer::placeObject(
        const TextObject& object,
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObject(
            object,
            region,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    PlacementResult PageComposer::placeObjectInRegion(
        const TextObject& object,
        std::string_view regionName,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return writeObjectInRegion(
            object,
            regionName,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    void PageComposer::writeText(int x, int y, std::u32string_view text, const Style& style)
    {
        writeText(x, y, text, std::optional<Style>(style));
    }

    void PageComposer::writeText(
        int x,
        int y,
        std::u32string_view text,
        const std::optional<Style>& styleOverride)
    {
        if (y < 0 || y >= m_composedBuffer.getHeight())
        {
            return;
        }

        writeSegmentedLine(x, y, extractFirstLine(text), styleOverride);
        synchronizeTarget();
    }

    void PageComposer::writeTextUtf8(
        int x,
        int y,
        std::string_view utf8Text,
        const Style& style)
    {
        writeText(x, y, UnicodeConversion::utf8ToU32(utf8Text), style);
    }

    void PageComposer::writeTextUtf8(
        int x,
        int y,
        std::string_view utf8Text,
        const std::optional<Style>& styleOverride)
    {
        writeText(x, y, UnicodeConversion::utf8ToU32(utf8Text), styleOverride);
    }

    void PageComposer::writeTextBlock(
        int x,
        int y,
        std::u32string_view block,
        const Style& style)
    {
        writeTextBlock(x, y, block, std::optional<Style>(style));
    }

    void PageComposer::writeTextBlock(
        int x,
        int y,
        std::u32string_view block,
        const std::optional<Style>& styleOverride)
    {
        if (m_composedBuffer.getWidth() <= 0 || m_composedBuffer.getHeight() <= 0)
        {
            return;
        }

        const std::vector<std::u32string> lines = splitLines(block);
        int currentY = y;

        for (const std::u32string& line : lines)
        {
            if (currentY >= m_composedBuffer.getHeight())
            {
                break;
            }

            if (currentY >= 0)
            {
                writeSegmentedLine(x, currentY, line, styleOverride);
            }

            ++currentY;
        }

        synchronizeTarget();
    }

    void PageComposer::writeTextBlockUtf8(
        int x,
        int y,
        std::string_view utf8Block,
        const Style& style)
    {
        writeTextBlock(x, y, UnicodeConversion::utf8ToU32(utf8Block), style);
    }

    void PageComposer::writeTextBlockUtf8(
        int x,
        int y,
        std::string_view utf8Block,
        const std::optional<Style>& styleOverride)
    {
        writeTextBlock(x, y, UnicodeConversion::utf8ToU32(utf8Block), styleOverride);
    }

    Point PageComposer::drawObjectAt(
        const TextObject& object,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        Point origin;
        origin.x = x;
        origin.y = y;

        if (!object.isLoaded())
        {
            return origin;
        }

        object.draw(m_composedBuffer, x, y, writePolicy, overrideStyle);
        synchronizeTarget();
        return origin;
    }

    PlacementResult PageComposer::makeUnresolvedPlacementResult(
        const TextObject& object,
        const Alignment& alignment)
    {
        PlacementResult result;
        result.origin = Point{};
        result.region = makeRect(0, 0, 0, 0);
        result.contentSize = measureObject(object);
        result.alignment = alignment;
        result.clamped = false;
        return result;
    }

    void PageComposer::writeSegmentedLine(
        int x,
        int y,
        std::u32string_view line,
        const std::optional<Style>& styleOverride)
    {
        if (m_composedBuffer.getWidth() <= 0 ||
            m_composedBuffer.getHeight() <= 0 ||
            y < 0 ||
            y >= m_composedBuffer.getHeight())
        {
            return;
        }

        int cursorX = x;
        const std::vector<TextCluster> clusters = GraphemeSegmentation::segment(line);

        for (const TextCluster& cluster : clusters)
        {
            if (cluster.codePoints.empty())
            {
                continue;
            }

            if (cursorX >= m_composedBuffer.getWidth())
            {
                break;
            }

            for (char32_t codePoint : cluster.codePoints)
            {
                if (cursorX < 0)
                {
                    continue;
                }

                m_composedBuffer.writeCodePoint(cursorX, y, codePoint, styleOverride);
            }

            if (cluster.displayWidth > 0)
            {
                cursorX += cluster.displayWidth;
            }
        }
    }

    std::u32string PageComposer::extractFirstLine(std::u32string_view text)
    {
        std::u32string line;
        line.reserve(text.size());

        for (char32_t ch : text)
        {
            if (isLineBreak(ch))
            {
                break;
            }

            line.push_back(ch);
        }

        return line;
    }

    std::vector<std::u32string> PageComposer::splitLines(std::u32string_view text)
    {
        std::vector<std::u32string> lines;
        std::u32string current;
        current.reserve(text.size());

        for (std::size_t i = 0; i < text.size(); ++i)
        {
            const char32_t ch = text[i];

            if (ch == U'\r')
            {
                if (i + 1 < text.size() && text[i + 1] == U'\n')
                {
                    ++i;
                }

                lines.push_back(current);
                current.clear();
                continue;
            }

            if (ch == U'\n')
            {
                lines.push_back(current);
                current.clear();
                continue;
            }

            current.push_back(ch);
        }

        lines.push_back(current);
        return lines;
    }

    void PageComposer::synchronizeTarget()
    {
        if (m_target == nullptr)
        {
            return;
        }

        *m_target = m_composedBuffer;
    }
}
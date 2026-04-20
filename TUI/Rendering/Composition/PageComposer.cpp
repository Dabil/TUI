#include "Rendering/Composition/PageComposer.h"

#include "Assets/AssetLibrary.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>
#include <map>

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

        recordOperation(
            PageCompositionDiagnostics::OperationKind::Resize,
            "resize",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            true);
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

        recordOperation(
            PageCompositionDiagnostics::OperationKind::Clear,
            "clear",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            true);
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
            recordOperation(
                PageCompositionDiagnostics::OperationKind::LoadScreen,
                "loadScreen",
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                &writePolicy,
                false,
                overrideStyle.has_value(),
                false,
                false,
                false,
                {},
                {},
                "Screen object is not loaded.");
            return false;
        }

        object.draw(m_composedBuffer, 0, 0, writePolicy, overrideStyle);
        synchronizeTarget();
        recordOperation(
            PageCompositionDiagnostics::OperationKind::LoadScreen,
            "loadScreen",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            &writePolicy,
            false,
            overrideStyle.has_value(),
            false,
            false,
            true);
        return true;
    }

    bool PageComposer::createRegion(
        int x,
        int y,
        int width,
        int height,
        std::string_view name)
    {
        const Rect region = makeRect(x, y, width, height);
        const bool success = m_regions.createRegion(x, y, width, height, name);
        recordOperation(
            PageCompositionDiagnostics::OperationKind::CreateRegion,
            "createRegion",
            &region,
            &region,
            nullptr,
            &region.size,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            success,
            name,
            {},
            success ? std::string_view{} : std::string_view{ "Unable to create region." });
        return success;
    }

    bool PageComposer::createRegion(const Rect& rect, std::string_view name)
    {
        return createRegion(
            rect.position.x,
            rect.position.y,
            rect.size.width,
            rect.size.height,
            name);
    }

    bool PageComposer::createFullScreenRegion(std::string_view name)
    {
        return createRegion(getFullScreenRegion(), name);
    }

    bool PageComposer::createCenteredRegion(
        int width,
        int height,
        std::string_view name)
    {
        return createCenteredRegion(getFullScreenRegion(), width, height, name);
    }

    bool PageComposer::createCenteredRegion(
        const Rect& container,
        int width,
        int height,
        std::string_view name)
    {
        const int x = container.position.x + (container.size.width - width) / 2;
        const int y = container.position.y + (container.size.height - height) / 2;

        return createRegion(x, y, width, height, name);
    }

    bool PageComposer::createCenteredRegion(
        std::string_view containerRegionName,
        int width,
        int height,
        std::string_view name)
    {
        const NamedRegion* containerRegion = getRegion(containerRegionName);
        if (containerRegion == nullptr)
        {
            const Rect requestedRegion = makeRect(0, 0, width, height);
            recordOperation(
                PageCompositionDiagnostics::OperationKind::CreateRegion,
                "createCenteredRegion",
                &requestedRegion,
                nullptr,
                nullptr,
                &requestedRegion.size,
                nullptr,
                nullptr,
                false,
                false,
                false,
                false,
                false,
                name,
                containerRegionName,
                "Container region was not found.");
            return false;
        }

        return createCenteredRegion(containerRegion->bounds, width, height, name);
    }

    void PageComposer::fillRegion(const Rect& target, char32_t glyph, const Style& style)
    {
        if (target.size.width <= 0 || target.size.height <= 0)
        {
            return;
        }

        const std::optional<Style> styleOverride(style);

        for (int row = 0; row < target.size.height; ++row)
        {
            const int y = target.position.y + row;
            for (int col = 0; col < target.size.width; ++col)
            {
                const int x = target.position.x + col;
                m_composedBuffer.writeCodePoint(x, y, glyph, styleOverride);
            }
        }

        synchronizeTarget();

        const Point origin{ target.position.x, target.position.y };
        recordOperation(
            PageCompositionDiagnostics::OperationKind::WriteTextBlock,
            "fillRegion",
            &target,
            &target,
            &origin,
            &target.size,
            nullptr,
            nullptr,
            false,
            true,
            false,
            false,
            true);
    }

    void PageComposer::fillRegion(
        std::string_view targetRegionName,
        char32_t glyph,
        const Style& style)
    {
        const NamedRegion* region = getRegion(targetRegionName);
        if (region == nullptr)
        {
            return;
        }

        fillRegion(region->bounds, glyph, style);
    }

    void PageComposer::styleRegion(const Rect& target, const Style& style)
    {
        if (target.size.width <= 0 || target.size.height <= 0)
        {
            return;
        }

        const std::optional<Style> styleOverride(style);

        for (int row = 0; row < target.size.height; ++row)
        {
            const int y = target.position.y + row;
            for (int col = 0; col < target.size.width; ++col)
            {
                const int x = target.position.x + col;
                const char32_t existingGlyph = m_composedBuffer.getDisplayGlyph(x, y);
                m_composedBuffer.writeCodePoint(x, y, existingGlyph, styleOverride);
            }
        }

        synchronizeTarget();

        const Point origin{ target.position.x, target.position.y };
        recordOperation(
            PageCompositionDiagnostics::OperationKind::WriteTextBlock,
            "styleRegion",
            &target,
            &target,
            &origin,
            &target.size,
            nullptr,
            nullptr,
            false,
            true,
            false,
            false,
            true);
    }

    void PageComposer::styleRegion(std::string_view targetRegionName, const Style& style)
    {
        const NamedRegion* region = getRegion(targetRegionName);
        if (region == nullptr)
        {
            return;
        }

        styleRegion(region->bounds, style);
    }

    void PageComposer::clearRegion(const Rect& target, const Style& style)
    {
        fillRegion(target, U' ', style);
    }

    void PageComposer::clearRegion(std::string_view targetRegionName, const Style& style)
    {
        const NamedRegion* region = getRegion(targetRegionName);
        if (region == nullptr)
        {
            return;
        }

        clearRegion(region->bounds, style);
    }

    void PageComposer::drawFrame(const Rect& target, const Style& style)
    {
        drawFrame(
            target.position.x,
            target.position.y,
            target.size.width,
            target.size.height,
            style);
    }

    void PageComposer::drawFrame(std::string_view targetRegionName, const Style& style)
    {
        const NamedRegion* region = getRegion(targetRegionName);
        if (region == nullptr)
        {
            return;
        }

        drawFrame(region->bounds, style);
    }

    void PageComposer::drawFrame(int x, int y, int width, int height, const Style& style)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        const std::optional<Style> styleOverride(style);
        const Rect target = makeRect(x, y, width, height);

        if (width == 1 && height == 1)
        {
            m_composedBuffer.writeCodePoint(x, y, U'•', styleOverride);
            synchronizeTarget();

            const Point origin{ x, y };
            const Size contentSize{ width, height };
            recordOperation(
                PageCompositionDiagnostics::OperationKind::WriteTextBlock,
                "drawFrame",
                &target,
                &target,
                &origin,
                &contentSize,
                nullptr,
                nullptr,
                false,
                true,
                false,
                false,
                true);
            return;
        }

        if (height == 1)
        {
            for (int col = 0; col < width; ++col)
            {
                m_composedBuffer.writeCodePoint(x + col, y, U'─', styleOverride);
            }

            synchronizeTarget();

            const Point origin{ x, y };
            const Size contentSize{ width, height };
            recordOperation(
                PageCompositionDiagnostics::OperationKind::WriteTextBlock,
                "drawFrame",
                &target,
                &target,
                &origin,
                &contentSize,
                nullptr,
                nullptr,
                false,
                true,
                false,
                false,
                true);
            return;
        }

        if (width == 1)
        {
            for (int row = 0; row < height; ++row)
            {
                m_composedBuffer.writeCodePoint(x, y + row, U'│', styleOverride);
            }

            synchronizeTarget();

            const Point origin{ x, y };
            const Size contentSize{ width, height };
            recordOperation(
                PageCompositionDiagnostics::OperationKind::WriteTextBlock,
                "drawFrame",
                &target,
                &target,
                &origin,
                &contentSize,
                nullptr,
                nullptr,
                false,
                true,
                false,
                false,
                true);
            return;
        }

        m_composedBuffer.writeCodePoint(x, y, U'┌', styleOverride);
        m_composedBuffer.writeCodePoint(x + width - 1, y, U'┐', styleOverride);
        m_composedBuffer.writeCodePoint(x, y + height - 1, U'└', styleOverride);
        m_composedBuffer.writeCodePoint(x + width - 1, y + height - 1, U'┘', styleOverride);

        for (int col = 1; col < width - 1; ++col)
        {
            m_composedBuffer.writeCodePoint(x + col, y, U'─', styleOverride);
            m_composedBuffer.writeCodePoint(x + col, y + height - 1, U'─', styleOverride);
        }

        for (int row = 1; row < height - 1; ++row)
        {
            m_composedBuffer.writeCodePoint(x, y + row, U'│', styleOverride);
            m_composedBuffer.writeCodePoint(x + width - 1, y + row, U'│', styleOverride);
        }

        synchronizeTarget();

        const Point origin{ x, y };
        const Size contentSize{ width, height };
        recordOperation(
            PageCompositionDiagnostics::OperationKind::WriteTextBlock,
            "drawFrame",
            &target,
            &target,
            &origin,
            &contentSize,
            nullptr,
            nullptr,
            false,
            true,
            false,
            false,
            true);
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
        recordOperation(
            PageCompositionDiagnostics::OperationKind::ClearRegions,
            "clearRegions",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            true);
    }

    Rect PageComposer::getFullScreenRegion() const
    {
        return makeRect(0, 0, std::max(0, getWidth()), std::max(0, getHeight()));
    }

    Rect PageComposer::peekTop(const Rect& rect, int height) const
    {
        const int clampedHeight = std::max(0, std::min(height, rect.size.height));
        return makeRect(
            rect.position.x,
            rect.position.y,
            rect.size.width,
            clampedHeight);
    }

    Rect PageComposer::peekBottom(const Rect& rect, int height) const
    {
        const int clampedHeight = std::max(0, std::min(height, rect.size.height));
        return makeRect(
            rect.position.x,
            rect.position.y + (rect.size.height - clampedHeight),
            rect.size.width,
            clampedHeight);
    }

    Rect PageComposer::peekLeft(const Rect& rect, int width) const
    {
        const int clampedWidth = std::max(0, std::min(width, rect.size.width));
        return makeRect(
            rect.position.x,
            rect.position.y,
            clampedWidth,
            rect.size.height);
    }

    Rect PageComposer::peekRight(const Rect& rect, int width) const
    {
        const int clampedWidth = std::max(0, std::min(width, rect.size.width));
        return makeRect(
            rect.position.x + (rect.size.width - clampedWidth),
            rect.position.y,
            clampedWidth,
            rect.size.height);
    }

    Rect PageComposer::peekTop(std::string_view regionName, int height) const
    {
        const NamedRegion* region = getRegion(regionName);
        return region != nullptr ? peekTop(region->bounds, height) : Rect{};
    }

    Rect PageComposer::peekBottom(std::string_view regionName, int height) const
    {
        const NamedRegion* region = getRegion(regionName);
        return region != nullptr ? peekBottom(region->bounds, height) : Rect{};
    }

    Rect PageComposer::peekLeft(std::string_view regionName, int width) const
    {
        const NamedRegion* region = getRegion(regionName);
        return region != nullptr ? peekLeft(region->bounds, width) : Rect{};
    }

    Rect PageComposer::peekRight(std::string_view regionName, int width) const
    {
        const NamedRegion* region = getRegion(regionName);
        return region != nullptr ? peekRight(region->bounds, width) : Rect{};
    }

    Rect PageComposer::remainderBelow(const Rect& source, int consumedHeight) const
    {
        const int clampedHeight = std::max(0, std::min(consumedHeight, source.size.height));
        return makeRect(
            source.position.x,
            source.position.y + clampedHeight,
            source.size.width,
            source.size.height - clampedHeight);
    }

    Rect PageComposer::remainderAbove(const Rect& source, int consumedHeight) const
    {
        const int clampedHeight = std::max(0, std::min(consumedHeight, source.size.height));
        return makeRect(
            source.position.x,
            source.position.y,
            source.size.width,
            source.size.height - clampedHeight);
    }

    Rect PageComposer::remainderRightOf(const Rect& source, int consumedWidth) const
    {
        const int clampedWidth = std::max(0, std::min(consumedWidth, source.size.width));
        return makeRect(
            source.position.x + clampedWidth,
            source.position.y,
            source.size.width - clampedWidth,
            source.size.height);
    }

    Rect PageComposer::remainderLeftOf(const Rect& source, int consumedWidth) const
    {
        const int clampedWidth = std::max(0, std::min(consumedWidth, source.size.width));
        return makeRect(
            source.position.x,
            source.position.y,
            source.size.width - clampedWidth,
            source.size.height);
    }

    Rect PageComposer::remainderBelow(std::string_view sourceRegionName, int consumedHeight) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? remainderBelow(region->bounds, consumedHeight) : Rect{};
    }

    Rect PageComposer::remainderAbove(std::string_view sourceRegionName, int consumedHeight) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? remainderAbove(region->bounds, consumedHeight) : Rect{};
    }

    Rect PageComposer::remainderRightOf(std::string_view sourceRegionName, int consumedWidth) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? remainderRightOf(region->bounds, consumedWidth) : Rect{};
    }

    Rect PageComposer::remainderLeftOf(std::string_view sourceRegionName, int consumedWidth) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? remainderLeftOf(region->bounds, consumedWidth) : Rect{};
    }

    std::pair<Rect, Rect> PageComposer::splitTop(const Rect& source, int height) const
    {
        return {
            peekTop(source, height),
            remainderBelow(source, height)
        };
    }

    std::pair<Rect, Rect> PageComposer::splitBottom(const Rect& source, int height) const
    {
        return {
            peekBottom(source, height),
            remainderAbove(source, height)
        };
    }

    std::pair<Rect, Rect> PageComposer::splitLeft(const Rect& source, int width) const
    {
        return {
            peekLeft(source, width),
            remainderRightOf(source, width)
        };
    }

    std::pair<Rect, Rect> PageComposer::splitRight(const Rect& source, int width) const
    {
        return {
            peekRight(source, width),
            remainderLeftOf(source, width)
        };
    }

    std::pair<Rect, Rect> PageComposer::splitTop(std::string_view sourceRegionName, int height) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return { Rect{}, Rect{} };
        }

        return splitTop(region->bounds, height);
    }

    std::pair<Rect, Rect> PageComposer::splitBottom(std::string_view sourceRegionName, int height) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return { Rect{}, Rect{} };
        }

        return splitBottom(region->bounds, height);
    }

    std::pair<Rect, Rect> PageComposer::splitLeft(std::string_view sourceRegionName, int width) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return { Rect{}, Rect{} };
        }

        return splitLeft(region->bounds, width);
    }

    std::pair<Rect, Rect> PageComposer::splitRight(std::string_view sourceRegionName, int width) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return { Rect{}, Rect{} };
        }

        return splitRight(region->bounds, width);
    }

    std::vector<Rect> PageComposer::splitGrid(const Rect& source, int rows, int cols) const
    {
        std::vector<Rect> cells;

        if (rows <= 0 || cols <= 0 || source.size.width < 0 || source.size.height < 0)
        {
            return cells;
        }

        cells.reserve(static_cast<std::size_t>(rows * cols));

        const int baseCellWidth = source.size.width / cols;
        const int extraWidth = source.size.width % cols;

        const int baseCellHeight = source.size.height / rows;
        const int extraHeight = source.size.height % rows;

        int y = source.position.y;
        for (int row = 0; row < rows; ++row)
        {
            const int cellHeight = baseCellHeight + (row < extraHeight ? 1 : 0);

            int x = source.position.x;
            for (int col = 0; col < cols; ++col)
            {
                const int cellWidth = baseCellWidth + (col < extraWidth ? 1 : 0);

                cells.push_back(makeRect(x, y, cellWidth, cellHeight));
                x += cellWidth;
            }

            y += cellHeight;
        }

        return cells;
    }

    std::vector<Rect> PageComposer::splitGrid(
        std::string_view sourceRegionName,
        int rows,
        int cols) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? splitGrid(region->bounds, rows, cols) : std::vector<Rect>{};
    }

    Rect PageComposer::gridCell(
        const Rect& source,
        int rows,
        int cols,
        int row,
        int col) const
    {
        if (rows <= 0 || cols <= 0 || row < 0 || col < 0 || row >= rows || col >= cols)
        {
            return Rect{};
        }

        const std::vector<Rect> cells = splitGrid(source, rows, cols);
        const std::size_t index = static_cast<std::size_t>(row * cols + col);

        return index < cells.size() ? cells[index] : Rect{};
    }

    Rect PageComposer::gridCell(
        std::string_view sourceRegionName,
        int rows,
        int cols,
        int row,
        int col) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? gridCell(region->bounds, rows, cols, row, col) : Rect{};
    }

    std::vector<Rect> PageComposer::gridRow(
        const Rect& source,
        int rows,
        int cols,
        int row) const
    {
        std::vector<Rect> result;

        if (rows <= 0 || cols <= 0 || row < 0 || row >= rows)
        {
            return result;
        }

        const std::vector<Rect> cells = splitGrid(source, rows, cols);
        result.reserve(static_cast<std::size_t>(cols));

        const std::size_t startIndex = static_cast<std::size_t>(row * cols);
        for (int col = 0; col < cols; ++col)
        {
            const std::size_t index = startIndex + static_cast<std::size_t>(col);
            if (index < cells.size())
            {
                result.push_back(cells[index]);
            }
        }

        return result;
    }

    std::vector<Rect> PageComposer::gridRow(
        std::string_view sourceRegionName,
        int rows,
        int cols,
        int row) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? gridRow(region->bounds, rows, cols, row) : std::vector<Rect>{};
    }

    std::vector<Rect> PageComposer::gridColumn(
        const Rect& source,
        int rows,
        int cols,
        int col) const
    {
        std::vector<Rect> result;

        if (rows <= 0 || cols <= 0 || col < 0 || col >= cols)
        {
            return result;
        }

        const std::vector<Rect> cells = splitGrid(source, rows, cols);
        result.reserve(static_cast<std::size_t>(rows));

        for (int row = 0; row < rows; ++row)
        {
            const std::size_t index = static_cast<std::size_t>(row * cols + col);
            if (index < cells.size())
            {
                result.push_back(cells[index]);
            }
        }

        return result;
    }

    std::vector<Rect> PageComposer::gridColumn(
        std::string_view sourceRegionName,
        int rows,
        int cols,
        int col) const
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        return region != nullptr ? gridColumn(region->bounds, rows, cols, col) : std::vector<Rect>{};
    }

    bool PageComposer::registerGrid(
        const Rect& source,
        int rows,
        int cols,
        const std::vector<std::string>& names)
    {
        if (rows <= 0 || cols <= 0)
        {
            return false;
        }

        const std::size_t expectedCount = static_cast<std::size_t>(rows * cols);
        if (names.size() != expectedCount)
        {
            return false;
        }

        const std::vector<Rect> cells = splitGrid(source, rows, cols);
        if (cells.size() != expectedCount)
        {
            return false;
        }

        for (std::size_t i = 0; i < cells.size(); ++i)
        {
            if (!createRegion(cells[i], names[i]))
            {
                return false;
            }
        }

        return true;
    }

    bool PageComposer::registerGrid(
        std::string_view sourceRegionName,
        int rows,
        int cols,
        const std::vector<std::string>& names)
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return false;
        }

        return registerGrid(region->bounds, rows, cols, names);
    }

    bool PageComposer::registerGrid(
        const Rect& source,
        int rows,
        int cols,
        std::string_view namePrefix)
    {
        if (rows <= 0 || cols <= 0)
        {
            return false;
        }

        const std::vector<Rect> cells = splitGrid(source, rows, cols);
        const std::size_t expectedCount = static_cast<std::size_t>(rows * cols);
        if (cells.size() != expectedCount)
        {
            return false;
        }

        std::size_t index = 0;
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                const std::string name =
                    std::string(namePrefix) +
                    "_r" + std::to_string(row) +
                    "_c" + std::to_string(col);

                if (!createRegion(cells[index], name))
                {
                    return false;
                }

                ++index;
            }
        }

        return true;
    }

    bool PageComposer::registerGrid(
        std::string_view sourceRegionName,
        int rows,
        int cols,
        std::string_view namePrefix)
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return false;
        }

        return registerGrid(region->bounds, rows, cols, namePrefix);
    }

    bool PageComposer::registerGridArea(
        const Rect& source,
        int rows,
        int cols,
        const std::vector<std::string>& areaNames)
    {
        if (rows <= 0 || cols <= 0)
        {
            return false;
        }

        const std::size_t expectedCount = static_cast<std::size_t>(rows * cols);
        if (areaNames.size() != expectedCount)
        {
            return false;
        }

        const std::vector<Rect> cells = splitGrid(source, rows, cols);
        if (cells.size() != expectedCount)
        {
            return false;
        }

        struct AreaBounds
        {
            int minRow = 0;
            int maxRow = 0;
            int minCol = 0;
            int maxCol = 0;
            bool initialized = false;
        };

        std::map<std::string, AreaBounds> areas;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                const std::size_t index = static_cast<std::size_t>(row * cols + col);
                const std::string& name = areaNames[index];

                if (name.empty())
                {
                    return false;
                }

                AreaBounds& bounds = areas[name];
                if (!bounds.initialized)
                {
                    bounds.minRow = row;
                    bounds.maxRow = row;
                    bounds.minCol = col;
                    bounds.maxCol = col;
                    bounds.initialized = true;
                }
                else
                {
                    bounds.minRow = std::min(bounds.minRow, row);
                    bounds.maxRow = std::max(bounds.maxRow, row);
                    bounds.minCol = std::min(bounds.minCol, col);
                    bounds.maxCol = std::max(bounds.maxCol, col);
                }
            }
        }

        for (const auto& entry : areas)
        {
            const std::string& name = entry.first;
            const AreaBounds& bounds = entry.second;

            for (int row = bounds.minRow; row <= bounds.maxRow; ++row)
            {
                for (int col = bounds.minCol; col <= bounds.maxCol; ++col)
                {
                    const std::size_t index = static_cast<std::size_t>(row * cols + col);
                    if (areaNames[index] != name)
                    {
                        return false;
                    }
                }
            }

            const Rect& topLeft =
                cells[static_cast<std::size_t>(bounds.minRow * cols + bounds.minCol)];
            const Rect& bottomRight =
                cells[static_cast<std::size_t>(bounds.maxRow * cols + bounds.maxCol)];

            const int x = topLeft.position.x;
            const int y = topLeft.position.y;
            const int width =
                (bottomRight.position.x + bottomRight.size.width) - topLeft.position.x;
            const int height =
                (bottomRight.position.y + bottomRight.size.height) - topLeft.position.y;

            if (!createRegion(x, y, width, height, name))
            {
                return false;
            }
        }

        return true;
    }

    bool PageComposer::registerGridArea(
        std::string_view sourceRegionName,
        int rows,
        int cols,
        const std::vector<std::string>& areaNames)
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return false;
        }

        return registerGridArea(region->bounds, rows, cols, areaNames);
    }

    bool PageComposer::registerGridArea(
        const Rect& source,
        int rows,
        int cols,
        std::initializer_list<std::string> areaNames)
    {
        return registerGridArea(
            source,
            rows,
            cols,
            std::vector<std::string>(areaNames));
    }

    bool PageComposer::registerGridArea(
        std::string_view sourceRegionName,
        int rows,
        int cols,
        std::initializer_list<std::string> areaNames)
    {
        return registerGridArea(
            sourceRegionName,
            rows,
            cols,
            std::vector<std::string>(areaNames));
    }

    bool PageComposer::createInsetRegion(const Rect& source, int inset, std::string_view name)
    {
        return createInsetRegion(source, inset, inset, inset, inset, name);
    }

    bool PageComposer::createInsetRegion(
        std::string_view sourceRegionName,
        int inset,
        std::string_view name)
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return false;
        }

        return createInsetRegion(region->bounds, inset, name);
    }

    bool PageComposer::createInsetRegion(
        const Rect& source,
        int left,
        int top,
        int right,
        int bottom,
        std::string_view name)
    {
        const int clampedLeft = std::max(0, left);
        const int clampedTop = std::max(0, top);
        const int clampedRight = std::max(0, right);
        const int clampedBottom = std::max(0, bottom);

        const int insetX = source.position.x + clampedLeft;
        const int insetY = source.position.y + clampedTop;
        const int insetWidth = std::max(0, source.size.width - clampedLeft - clampedRight);
        const int insetHeight = std::max(0, source.size.height - clampedTop - clampedBottom);

        return createRegion(insetX, insetY, insetWidth, insetHeight, name);
    }

    bool PageComposer::createInsetRegion(
        std::string_view sourceRegionName,
        int left,
        int top,
        int right,
        int bottom,
        std::string_view name)
    {
        const NamedRegion* region = getRegion(sourceRegionName);
        if (region == nullptr)
        {
            return false;
        }

        return createInsetRegion(region->bounds, left, top, right, bottom, name);
    }

    bool PageComposer::insetAll(
        const std::vector<std::string>& sourceNames,
        int inset,
        const std::vector<std::string>& destNames)
    {
        if (sourceNames.size() != destNames.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < sourceNames.size(); ++i)
        {
            if (!createInsetRegion(sourceNames[i], inset, destNames[i]))
            {
                return false;
            }
        }

        return true;
    }

    bool PageComposer::insetAll(
        const std::vector<std::string>& sourceNames,
        int left,
        int top,
        int right,
        int bottom,
        const std::vector<std::string>& destNames)
    {
        if (sourceNames.size() != destNames.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < sourceNames.size(); ++i)
        {
            if (!createInsetRegion(
                sourceNames[i],
                left,
                top,
                right,
                bottom,
                destNames[i]))
            {
                return false;
            }
        }

        return true;
    }

    void PageComposer::setAssetLibrary(Assets::AssetLibrary& assetLibrary)
    {
        m_assetLibrary = &assetLibrary;
    }

    void PageComposer::detachAssetLibrary()
    {
        m_assetLibrary = nullptr;
    }

    bool PageComposer::hasAssetLibrary() const
    {
        return m_assetLibrary != nullptr;
    }

    Assets::AssetLibrary* PageComposer::tryGetAssetLibrary()
    {
        return m_assetLibrary;
    }

    const Assets::AssetLibrary* PageComposer::tryGetAssetLibrary() const
    {
        return m_assetLibrary;
    }

    void PageComposer::setFrames(std::vector<TextObject> frames)
    {
        m_frames = std::move(frames);
        recordOperation(
            PageCompositionDiagnostics::OperationKind::SetFrames,
            "setFrames",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            true);
    }

    void PageComposer::clearFrames()
    {
        m_frames.clear();
        recordOperation(
            PageCompositionDiagnostics::OperationKind::ClearFrames,
            "clearFrames",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            true);
    }

    std::size_t PageComposer::getFrameCount() const
    {
        return m_frames.size();
    }

    bool PageComposer::hasFrame(int frameIndex) const
    {
        return getFrame(frameIndex) != nullptr;
    }

    const TextObject* PageComposer::getFrame(int frameIndex) const
    {
        if (frameIndex < 0 || static_cast<std::size_t>(frameIndex) >= m_frames.size())
        {
            return nullptr;
        }

        return &m_frames[static_cast<std::size_t>(frameIndex)];
    }


    void PageComposer::setDiagnostics(PageCompositionDiagnostics& diagnostics)
    {
        m_diagnostics = &diagnostics;
        refreshDeterministicSignature();
    }

    void PageComposer::detachDiagnostics()
    {
        m_diagnostics = nullptr;
    }

    bool PageComposer::hasDiagnostics() const
    {
        return m_diagnostics != nullptr;
    }

    PageCompositionDiagnostics* PageComposer::tryGetDiagnostics()
    {
        return m_diagnostics;
    }

    const PageCompositionDiagnostics* PageComposer::tryGetDiagnostics() const
    {
        return m_diagnostics;
    }

    void PageComposer::beginFrame(
        int frameIndex,
        std::string_view channel,
        std::string_view tag)
    {
        m_frameContext.frameIndex = frameIndex;
        m_frameContext.channel = std::string(channel);
        m_frameContext.tag = std::string(tag);

        recordOperation(
            PageCompositionDiagnostics::OperationKind::BeginFrame,
            "beginFrame",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            true,
            {},
            {},
            {});
        refreshDeterministicSignature();
    }

    void PageComposer::endFrame()
    {
        refreshDeterministicSignature();
        recordOperation(
            PageCompositionDiagnostics::OperationKind::EndFrame,
            "endFrame",
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            false,
            false,
            false,
            false,
            true,
            {},
            {},
            {});
    }

    void PageComposer::clearFrameContext()
    {
        m_frameContext = {};
        refreshDeterministicSignature();
    }

    const PageCompositionDiagnostics::FrameContext& PageComposer::frameContext() const
    {
        return m_frameContext;
    }

    std::uint64_t PageComposer::computeDeterministicSignature() const
    {
        return PageCompositionDiagnostics::computeStableSignature(
            getWidth(),
            getHeight(),
            renderToU32String(),
            m_frameContext);
    }

    bool PageComposer::verifyDeterministicSignature(std::uint64_t expectedSignature) const
    {
        return computeDeterministicSignature() == expectedSignature;
    }

    std::uint64_t PageComposer::lastDeterministicSignature() const
    {
        if (m_diagnostics != nullptr)
        {
            return m_diagnostics->lastDeterministicSignature();
        }

        return computeDeterministicSignature();
    }

    int PageComposer::centerX(int contentWidth) const
    {
        return Composition::centerX(getFullScreenRegion(), contentWidth);
    }

    int PageComposer::centerY(int contentHeight) const
    {
        return Composition::centerY(getFullScreenRegion(), contentHeight);
    }

    Point PageComposer::centerInFullScreen(const Size& contentSize) const
    {
        return Composition::centerInRegion(getFullScreenRegion(), contentSize);
    }

    Point PageComposer::anchorInFullScreen(AnchorPoint anchor) const
    {
        return anchorPointInRegion(getFullScreenRegion(), anchor);
    }

    int PageComposer::centerX(std::string_view regionName, int contentWidth) const
    {
        const NamedRegion* region = m_regions.getRegion(regionName);
        return region != nullptr ? Composition::centerX(region->bounds, contentWidth) : 0;
    }

    int PageComposer::centerY(std::string_view regionName, int contentHeight) const
    {
        const NamedRegion* region = m_regions.getRegion(regionName);
        return region != nullptr ? Composition::centerY(region->bounds, contentHeight) : 0;
    }

    Point PageComposer::centerInRegion(std::string_view regionName, const Size& contentSize) const
    {
        const NamedRegion* region = m_regions.getRegion(regionName);
        return region != nullptr ? Composition::centerInRegion(region->bounds, contentSize) : Point{};
    }

    Point PageComposer::anchorInRegion(std::string_view regionName, AnchorPoint anchor) const
    {
        const NamedRegion* region = m_regions.getRegion(regionName);
        return region != nullptr ? anchorPointInRegion(region->bounds, anchor) : Point{};
    }

    SourcePlacementResult PageComposer::placeSource(
        const ObjectSource& source,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return placeResolvedSource(
            resolveObjectSource(source, m_assetLibrary),
            x,
            y,
            writePolicy,
            overrideStyle);
    }

    SourcePlacementResult PageComposer::placeSource(
        const ObjectSource& source,
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeResolvedSource(
            resolveObjectSource(source, m_assetLibrary),
            region,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }


    SourcePlacementResult PageComposer::placeSource(
        const ObjectSource& source,
        const PlacementSpec& placement,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        switch (placement.mode)
        {
        case PlacementSpec::Mode::Point:
            return placeSource(
                source,
                placement.point.x,
                placement.point.y,
                writePolicy,
                overrideStyle);

        case PlacementSpec::Mode::Region:
            return placeSource(
                source,
                placement.region,
                placement.alignment,
                writePolicy,
                overrideStyle,
                placement.clampToRegion);

        case PlacementSpec::Mode::RegionName:
            return placeSourceInRegion(
                source,
                placement.regionName,
                placement.alignment,
                writePolicy,
                overrideStyle,
                placement.clampToRegion);

        case PlacementSpec::Mode::FullScreenAligned:
            return placeSourceAligned(
                source,
                placement.alignment,
                writePolicy,
                overrideStyle,
                placement.clampToRegion);

        default:
            return makeFailedSourcePlacement(resolveObjectSource(source, m_assetLibrary));
        }
    }

    SourcePlacementResult PageComposer::placeSourceInRegion(
        const ObjectSource& source,
        std::string_view regionName,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        const NamedRegion* region = m_regions.getRegion(regionName);
        if (region == nullptr)
        {
            return makeFailedSourcePlacement(
                resolveObjectSource(source, m_assetLibrary),
                &alignment);
        }

        return placeSource(
            source,
            region->bounds,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeSourceAligned(
        const ObjectSource& source,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSource(
            source,
            getFullScreenRegion(),
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeAsset(
        std::string_view assetName,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return placeSource(
            ObjectSource::fromAsset(std::string(assetName)),
            x,
            y,
            writePolicy,
            overrideStyle);
    }

    SourcePlacementResult PageComposer::placeAsset(
        std::string_view assetName,
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSource(
            ObjectSource::fromAsset(std::string(assetName)),
            region,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeAssetAligned(
        std::string_view assetName,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeAsset(
            assetName,
            getFullScreenRegion(),
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeFrame(
        int frameIndex,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return placeSource(
            ObjectSource::fromRegisteredFrame(m_frames, frameIndex),
            x,
            y,
            writePolicy,
            overrideStyle);
    }

    SourcePlacementResult PageComposer::placeFrame(
        int frameIndex,
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSource(
            ObjectSource::fromRegisteredFrame(m_frames, frameIndex),
            region,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeFrameAligned(
        int frameIndex,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeFrame(
            frameIndex,
            getFullScreenRegion(),
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }


    SourcePlacementResult PageComposer::placeActiveFrame(
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return placeFrame(resolveActiveFrameIndex(), x, y, writePolicy, overrideStyle);
    }

    SourcePlacementResult PageComposer::placeActiveFrame(
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeFrame(
            resolveActiveFrameIndex(),
            region,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeActiveFrameAligned(
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeFrameAligned(
            resolveActiveFrameIndex(),
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeSequenceFrame(
        std::string_view assetName,
        int frameIndex,
        int x,
        int y,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        ObjectSource source = ObjectSource::fromAssetFrame(std::string(assetName), frameIndex);
        source.xpFrameOptions = frameOptions;
        return placeSource(source, x, y, writePolicy, overrideStyle);
    }

    SourcePlacementResult PageComposer::placeSequenceFrame(
        std::string_view assetName,
        int frameIndex,
        const Rect& region,
        const Alignment& alignment,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        ObjectSource source = ObjectSource::fromAssetFrame(std::string(assetName), frameIndex);
        source.xpFrameOptions = frameOptions;
        return placeSource(
            source,
            region,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeSequenceFrameAligned(
        std::string_view assetName,
        int frameIndex,
        const Alignment& alignment,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSequenceFrame(
            assetName,
            frameIndex,
            getFullScreenRegion(),
            alignment,
            frameOptions,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }


    SourcePlacementResult PageComposer::placeActiveSequenceFrame(
        std::string_view assetName,
        int x,
        int y,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return placeSequenceFrame(
            assetName,
            m_frameContext.frameIndex,
            x,
            y,
            frameOptions,
            writePolicy,
            overrideStyle);
    }

    SourcePlacementResult PageComposer::placeActiveSequenceFrame(
        std::string_view assetName,
        const Rect& region,
        const Alignment& alignment,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSequenceFrame(
            assetName,
            m_frameContext.frameIndex,
            region,
            alignment,
            frameOptions,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeActiveSequenceFrameAligned(
        std::string_view assetName,
        const Alignment& alignment,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSequenceFrameAligned(
            assetName,
            m_frameContext.frameIndex,
            alignment,
            frameOptions,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeXpDocument(
        const XpArtLoader::XpDocument& document,
        int x,
        int y,
        const XpArtLoader::LoadOptions& loadOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return placeSource(
            ObjectSource::fromXpDocument(document, loadOptions),
            x,
            y,
            writePolicy,
            overrideStyle);
    }

    SourcePlacementResult PageComposer::placeXpFrame(
        const XpArtLoader::XpFrame& frame,
        int x,
        int y,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return placeSource(
            ObjectSource::fromXpFrame(frame, frameOptions),
            x,
            y,
            writePolicy,
            overrideStyle);
    }

    SourcePlacementResult PageComposer::placeXpDocumentAligned(
        const XpArtLoader::XpDocument& document,
        const Alignment& alignment,
        const XpArtLoader::LoadOptions& loadOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSourceAligned(
            ObjectSource::fromXpDocument(document, loadOptions),
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
    }

    SourcePlacementResult PageComposer::placeXpFrameAligned(
        const XpArtLoader::XpFrame& frame,
        const Alignment& alignment,
        const XpArtLoader::XpFrameConversionOptions& frameOptions,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        return placeSourceAligned(
            ObjectSource::fromXpFrame(frame, frameOptions),
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
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


    PlacementResult PageComposer::writeObject(
        const TextObject& object,
        const PlacementSpec& placement,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        switch (placement.mode)
        {
        case PlacementSpec::Mode::Point:
        {
            PlacementResult result;
            result.origin = writeObject(
                object,
                placement.point.x,
                placement.point.y,
                writePolicy,
                overrideStyle);
            result.region = makeRect(placement.point.x, placement.point.y, 0, 0);
            result.contentSize = measureObject(object);
            result.alignment = Align::topLeft();
            result.clamped = false;
            return result;
        }

        case PlacementSpec::Mode::Region:
            return writeObject(
                object,
                placement.region,
                placement.alignment,
                writePolicy,
                overrideStyle,
                placement.clampToRegion);

        case PlacementSpec::Mode::RegionName:
            return writeObjectInRegion(
                object,
                placement.regionName,
                placement.alignment,
                writePolicy,
                overrideStyle,
                placement.clampToRegion);

        case PlacementSpec::Mode::FullScreenAligned:
            return writeObjectAligned(
                object,
                placement.alignment,
                writePolicy,
                overrideStyle,
                placement.clampToRegion);

        default:
            return makeUnresolvedPlacementResult(object, Align::topLeft());
        }
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


    PlacementResult PageComposer::placeObject(
        const TextObject& object,
        const PlacementSpec& placement,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        return writeObject(object, placement, writePolicy, overrideStyle);
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

        const std::u32string line = extractFirstLine(text);
        writeSegmentedLine(x, y, line, styleOverride);
        synchronizeTarget();

        const Point origin{ x, y };
        const Size contentSize{ static_cast<int>(line.size()), 1 };
        recordOperation(
            PageCompositionDiagnostics::OperationKind::WriteTextLine,
            "writeText",
            nullptr,
            nullptr,
            &origin,
            &contentSize,
            nullptr,
            nullptr,
            false,
            styleOverride.has_value(),
            false,
            false,
            true);
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
        int maxWidth = 0;

        for (const std::u32string& line : lines)
        {
            maxWidth = std::max(maxWidth, static_cast<int>(line.size()));

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

        const Point origin{ x, y };
        const Size contentSize{ maxWidth, static_cast<int>(lines.size()) };
        recordOperation(
            PageCompositionDiagnostics::OperationKind::WriteTextBlock,
            "writeTextBlock",
            nullptr,
            nullptr,
            &origin,
            &contentSize,
            nullptr,
            nullptr,
            false,
            styleOverride.has_value(),
            false,
            false,
            true);
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


    SourcePlacementResult PageComposer::placeResolvedSource(
        const ResolvedObjectSource& resolvedSource,
        int x,
        int y,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle)
    {
        if (!resolvedSource.hasObject())
        {
            const SourcePlacementResult result = makeFailedSourcePlacement(resolvedSource);
            recordSourcePlacement(
                PageCompositionDiagnostics::OperationKind::PlaceSourceAtPoint,
                "placeSource",
                resolvedSource,
                nullptr,
                result,
                writePolicy,
                false,
                overrideStyle.has_value(),
                false);
            return result;
        }

        SourcePlacementResult result;
        result.source = resolvedSource;
        result.placement.origin = drawObjectAt(
            resolvedSource.object,
            x,
            y,
            writePolicy,
            overrideStyle);
        result.placement.region = makeRect(x, y, 0, 0);
        result.placement.contentSize = measureObject(resolvedSource.object);
        result.placement.alignment = Alignment{};
        result.placement.clamped = false;
        result.success = true;
        recordSourcePlacement(
            PageCompositionDiagnostics::OperationKind::PlaceSourceAtPoint,
            "placeSource",
            resolvedSource,
            nullptr,
            result,
            writePolicy,
            false,
            overrideStyle.has_value(),
            false);
        return result;
    }

    SourcePlacementResult PageComposer::placeResolvedSource(
        const ResolvedObjectSource& resolvedSource,
        const Rect& region,
        const Alignment& alignment,
        const WritePolicy& writePolicy,
        const std::optional<Style>& overrideStyle,
        bool clampToRegion)
    {
        if (!resolvedSource.hasObject())
        {
            const SourcePlacementResult result = makeFailedSourcePlacement(resolvedSource, &alignment);
            recordSourcePlacement(
                PageCompositionDiagnostics::OperationKind::PlaceSourceInRegion,
                "placeSource",
                resolvedSource,
                &region,
                result,
                writePolicy,
                true,
                overrideStyle.has_value(),
                clampToRegion);
            return result;
        }

        SourcePlacementResult result;
        result.source = resolvedSource;
        result.placement = writeObject(
            resolvedSource.object,
            region,
            alignment,
            writePolicy,
            overrideStyle,
            clampToRegion);
        result.success = true;
        recordSourcePlacement(
            PageCompositionDiagnostics::OperationKind::PlaceSourceInRegion,
            "placeSource",
            resolvedSource,
            &region,
            result,
            writePolicy,
            true,
            overrideStyle.has_value(),
            clampToRegion);
        return result;
    }

    SourcePlacementResult PageComposer::makeFailedSourcePlacement(
        const ResolvedObjectSource& resolvedSource,
        const Alignment* alignment)
    {
        SourcePlacementResult result;
        result.source = resolvedSource;
        result.success = false;
        result.placement.origin = Point{};
        result.placement.region = makeRect(0, 0, 0, 0);
        result.placement.contentSize = measureObject(resolvedSource.object);
        result.placement.alignment = alignment != nullptr ? *alignment : Alignment{};
        result.placement.clamped = false;
        return result;
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

    int PageComposer::resolveActiveFrameIndex() const
    {
        return m_frameContext.frameIndex;
    }

    void PageComposer::recordOperation(
        PageCompositionDiagnostics::OperationKind operation,
        std::string_view operationName,
        const Rect* requestedRegion,
        const Rect* resolvedRegion,
        const Point* origin,
        const Size* contentSize,
        const Alignment* alignment,
        const WritePolicy* writePolicy,
        bool usedAlignment,
        bool usedOverrideStyle,
        bool clampRequested,
        bool clamped,
        bool success,
        std::string_view regionName,
        std::string_view detail,
        std::string_view errorMessage)
    {
        if (m_diagnostics == nullptr)
        {
            return;
        }

        PageCompositionDiagnostics::Entry entry;
        entry.operation = operation;
        entry.operationName = std::string(operationName);
        entry.regionName = std::string(regionName);
        entry.detail = std::string(detail);
        entry.errorMessage = std::string(errorMessage);
        entry.usedAlignment = usedAlignment;
        entry.usedOverrideStyle = usedOverrideStyle;
        entry.clampRequested = clampRequested;
        entry.clamped = clamped;
        entry.success = success;
        entry.frameContext = m_frameContext;

        if (requestedRegion != nullptr)
        {
            entry.requestedRegion = *requestedRegion;
        }

        if (resolvedRegion != nullptr)
        {
            entry.resolvedRegion = *resolvedRegion;
        }

        if (origin != nullptr)
        {
            entry.origin = *origin;
        }

        if (contentSize != nullptr)
        {
            entry.contentSize = *contentSize;
        }

        if (alignment != nullptr)
        {
            entry.alignment = *alignment;
        }

        if (writePolicy != nullptr)
        {
            entry.writePolicy = *writePolicy;
        }

        m_diagnostics->record(std::move(entry));
    }

    void PageComposer::recordSourcePlacement(
        PageCompositionDiagnostics::OperationKind operation,
        std::string_view operationName,
        const ResolvedObjectSource& resolvedSource,
        const Rect* requestedRegion,
        const SourcePlacementResult& result,
        const WritePolicy& writePolicy,
        bool usedAlignment,
        bool usedOverrideStyle,
        bool clampRequested,
        std::string_view regionName)
    {
        if (m_diagnostics == nullptr)
        {
            return;
        }

        PageCompositionDiagnostics::Entry entry;
        entry.operation = operation;
        entry.operationName = std::string(operationName);
        entry.sourceKind = [&]() -> std::string
            {
                if (resolvedSource.usedAssetLibrary)
                {
                    return "asset";
                }
                if (resolvedSource.usedXpConversion)
                {
                    return "xp";
                }
                return "object";
            }();
        entry.regionName = std::string(regionName);
        entry.usedAlignment = usedAlignment;
        entry.usedOverrideStyle = usedOverrideStyle;
        entry.usedAssetLibrary = resolvedSource.usedAssetLibrary;
        entry.usedXpConversion = resolvedSource.usedXpConversion;
        entry.clampRequested = clampRequested;
        entry.clamped = result.placement.clamped;
        entry.success = result.success;
        entry.resolvedFrameIndex = resolvedSource.resolvedFrameIndex;
        entry.errorMessage = resolvedSource.errorMessage;
        entry.frameContext = m_frameContext;
        entry.requestedRegion = requestedRegion != nullptr ? *requestedRegion : Rect{};
        entry.resolvedRegion = result.placement.region;
        entry.origin = result.placement.origin;
        entry.contentSize = result.placement.contentSize;
        entry.alignment = result.placement.alignment;
        entry.writePolicy = writePolicy;
        m_diagnostics->record(std::move(entry));
    }

    void PageComposer::refreshDeterministicSignature()
    {
        if (m_diagnostics != nullptr)
        {
            m_diagnostics->setLastDeterministicSignature(computeDeterministicSignature());
        }
    }

    void PageComposer::synchronizeTarget()
    {
        if (m_target != nullptr)
        {
            *m_target = m_composedBuffer;
        }

        refreshDeterministicSignature();
    }
}


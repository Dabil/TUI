#pragma once

#include <functional>

#include "Core/Rect.h"
#include "Rendering/Surface.h"
#include "UI/Content/IWindowContent.h"

namespace Composition
{
    class PageComposer;
}

namespace UI
{
    class ComposedWindowContent final : public IWindowContent
    {
    public:
        using ComposeCallback =
            std::function<void(Composition::PageComposer& composer, const Rect& bounds)>;

        explicit ComposedWindowContent(ComposeCallback compose);

        ComposedWindowContent(const ComposedWindowContent&) = delete;
        ComposedWindowContent& operator=(const ComposedWindowContent&) = delete;

        ComposedWindowContent(ComposedWindowContent&&) noexcept = default;
        ComposedWindowContent& operator=(ComposedWindowContent&&) noexcept = default;

        ~ComposedWindowContent() override;

        void setComposeCallback(ComposeCallback compose);
        void requestRecompose();

        void update(const Animation::TickEvent& event) override;
        bool handleEvent(const Input::Event& event) override;
        void draw(Surface& surface, const Rect& bounds) override;
        void onAttached() override;
        void onDetached() override;
        void onBoundsChanged(const Rect& bounds) override;

    private:
        void resizeLocalSurface(const Rect& bounds);
        void rebuildIfDirty();

    private:
        ComposeCallback m_compose;
        Surface m_surface;
        Rect m_localBounds{};
        bool m_dirty = true;
        bool m_attached = false;
    };
}
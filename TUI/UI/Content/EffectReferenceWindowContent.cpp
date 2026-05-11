#include "UI/Content/EffectReferenceWindowContent.h"

#include "Rendering/Effects/IEffects.h"

namespace UI
{
    EffectReferenceWindowContent::EffectReferenceWindowContent(IEffect& effect)
        : m_effect(&effect)
    {
    }

    EffectReferenceWindowContent::~EffectReferenceWindowContent() = default;

    IEffect& EffectReferenceWindowContent::effect()
    {
        return *m_effect;
    }

    const IEffect& EffectReferenceWindowContent::effect() const
    {
        return *m_effect;
    }

    void EffectReferenceWindowContent::update(const Animation::TickEvent& event)
    {
        if (m_effect == nullptr)
        {
            return;
        }

        m_effect->update(event);
    }

    void EffectReferenceWindowContent::draw(Surface& surface, const Rect& bounds)
    {
        if (m_effect == nullptr)
        {
            return;
        }

        m_effect->draw(surface, bounds);
    }

    void EffectReferenceWindowContent::onAttached()
    {
        if (m_effect == nullptr)
        {
            return;
        }

        m_effect->onEnter();
    }

    void EffectReferenceWindowContent::onDetached()
    {
        if (m_effect == nullptr)
        {
            return;
        }

        m_effect->onExit();
    }

    void EffectReferenceWindowContent::onBoundsChanged(const Rect& bounds)
    {
        if (m_effect == nullptr)
        {
            return;
        }

        m_effect->onResize(bounds);
    }
}
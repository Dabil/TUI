#include "Animation/AnimationDrivenRecompositionLoop.h"

#include <utility>

namespace Animation
{
    void AnimationDrivenRecompositionLoop::setComposePageCallback(
        ComposePageCallback callback)
    {
        m_composePage = std::move(callback);
        requestRecompose();
    }

    void AnimationDrivenRecompositionLoop::clearComposePageCallback()
    {
        m_composePage = nullptr;
        requestRecompose();
    }

    void AnimationDrivenRecompositionLoop::setBindingResolver(
        AnimationBindingResolver* resolver)
    {
        m_bindingResolver = resolver;
        resetFrameTracking();
        requestRecompose();
    }

    void AnimationDrivenRecompositionLoop::clearBindingResolver()
    {
        m_bindingResolver = nullptr;
        resetFrameTracking();
        requestRecompose();
    }

    void AnimationDrivenRecompositionLoop::setInvalidationRegion(
        Rendering::InvalidationRegion* invalidation)
    {
        m_invalidation = invalidation;
    }

    void AnimationDrivenRecompositionLoop::clearInvalidationRegion()
    {
        m_invalidation = nullptr;
    }

    void AnimationDrivenRecompositionLoop::setClearStyle(const Style& style)
    {
        m_clearStyle = style;
        requestRecompose();
    }

    void AnimationDrivenRecompositionLoop::requestRecompose()
    {
        m_forceRecompose = true;
    }

    void AnimationDrivenRecompositionLoop::resetFrameTracking()
    {
        m_lastFrameState.clear();
    }

    AnimationDrivenRecompositionLoop::Result
        AnimationDrivenRecompositionLoop::updateAndRecomposeIfNeeded(
            ScreenBuffer& buffer,
            const TickEvent& event)
    {
        if (m_bindingResolver != nullptr)
        {
            m_bindingResolver->updateBoundControllers(event);
        }

        const std::vector<AnimationBindingFrameState> currentFrameState =
            m_bindingResolver != nullptr
            ? m_bindingResolver->captureFrameState()
            : std::vector<AnimationBindingFrameState>{};

        const bool frameStateChanged = shouldRecompose(currentFrameState);

        if (!m_forceRecompose && !frameStateChanged)
        {
            Result result;
            result.frameStateChanged = false;
            result.forced = false;
            result.recomposed = false;
            return result;
        }

        m_lastFrameState = currentFrameState;

        return composeIntoBuffer(
            buffer,
            m_forceRecompose,
            frameStateChanged);
    }

    AnimationDrivenRecompositionLoop::Result
        AnimationDrivenRecompositionLoop::recomposeNow(ScreenBuffer& buffer)
    {
        const std::vector<AnimationBindingFrameState> currentFrameState =
            m_bindingResolver != nullptr
            ? m_bindingResolver->captureFrameState()
            : std::vector<AnimationBindingFrameState>{};

        m_lastFrameState = currentFrameState;

        return composeIntoBuffer(
            buffer,
            true,
            true);
    }

    AnimationDrivenRecompositionLoop::Result
        AnimationDrivenRecompositionLoop::composeIntoBuffer(
            ScreenBuffer& buffer,
            bool forced,
            bool frameStateChanged)
    {
        Result result;
        result.recomposed = true;
        result.forced = forced;
        result.frameStateChanged = frameStateChanged;

        Composition::PageComposer composer(buffer);
        composer.clear(m_clearStyle);

        if (m_composePage)
        {
            m_composePage(composer);
        }

        if (m_bindingResolver != nullptr)
        {
            result.bindingResults = m_bindingResolver->resolveAll(composer);
        }

        result.deterministicSignature =
            composer.computeDeterministicSignature();

        if (m_invalidation != nullptr)
        {
            m_invalidation->invalidateAll();
        }

        m_forceRecompose = false;
        return result;
    }

    bool AnimationDrivenRecompositionLoop::shouldRecompose(
        const std::vector<AnimationBindingFrameState>& currentFrameState) const
    {
        if (m_forceRecompose)
        {
            return true;
        }

        return currentFrameState != m_lastFrameState;
    }
}
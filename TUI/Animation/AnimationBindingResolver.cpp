#include "Animation/AnimationBindingResolver.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace Animation
{
    AnimationBindingTarget AnimationBindingTarget::sequenceAssetPlacement(
        std::string_view targetNameValue,
        std::string_view controllerNameValue,
        std::string_view assetNameValue,
        const Composition::PageComposer::PlacementSpec& placementValue,
        const Composition::WritePolicy& policyValue)
    {
        AnimationBindingTarget target;
        target.kind = AnimationBindingTargetKind::SequenceAssetPlacement;
        target.targetName = std::string(targetNameValue);
        target.controllerName = std::string(controllerNameValue);
        target.assetName = std::string(assetNameValue);
        target.placement = placementValue;
        target.policy = policyValue;
        return target;
    }

    AnimationBindingTarget AnimationBindingTarget::framePlaceholder(
        std::string_view targetNameValue,
        std::string_view controllerNameValue,
        const std::vector<TextObject>& frames,
        const Composition::PageComposer::PlacementSpec& placementValue,
        const Composition::WritePolicy& policyValue)
    {
        AnimationBindingTarget target;
        target.kind = AnimationBindingTargetKind::FramePlaceholder;
        target.targetName = std::string(targetNameValue);
        target.controllerName = std::string(controllerNameValue);
        target.registeredFrames = &frames;
        target.placement = placementValue;
        target.policy = policyValue;
        return target;
    }

    AnimationBindingTarget AnimationBindingTarget::animatedTextSequence(
        std::string_view targetNameValue,
        std::string_view controllerNameValue,
        const AnimatedTextAssetSequence& sequence,
        const Composition::PageComposer::PlacementSpec& placementValue,
        const Composition::WritePolicy& policyValue)
    {
        AnimationBindingTarget target;
        target.kind = AnimationBindingTargetKind::RegisteredFrameSequence;
        target.targetName = std::string(targetNameValue);
        target.controllerName = std::string(controllerNameValue);
        target.textAssetSequence = &sequence;
        target.placement = placementValue;
        target.policy = policyValue;
        return target;
    }

    AnimationBindingTarget AnimationBindingTarget::xpSequencePlacement(
        std::string_view targetNameValue,
        std::string_view controllerNameValue,
        const XpArtLoader::XpSequence& sequence,
        const Composition::PageComposer::PlacementSpec& placementValue,
        const Composition::WritePolicy& policyValue,
        const XpArtLoader::XpFrameConversionOptions& frameOptions)
    {
        AnimationBindingTarget target;
        target.kind = AnimationBindingTargetKind::XpSequencePlacement;
        target.targetName = std::string(targetNameValue);
        target.controllerName = std::string(controllerNameValue);
        target.xpSequence = &sequence;
        target.xpFrameOptions = frameOptions;
        target.placement = placementValue;
        target.policy = policyValue;
        return target;
    }

    void AnimationBindingResolver::clear()
    {
        m_controllers.clear();
        m_targets.clear();
    }

    void AnimationBindingResolver::bindController(
        std::string_view name,
        Animator& animator)
    {
        if (name.empty())
        {
            return;
        }

        m_controllers[std::string(name)] = &animator;
    }

    void AnimationBindingResolver::unbindController(std::string_view name)
    {
        m_controllers.erase(std::string(name));
    }

    bool AnimationBindingResolver::hasController(std::string_view name) const
    {
        return tryGetController(name) != nullptr;
    }

    Animator* AnimationBindingResolver::tryGetController(std::string_view name)
    {
        const auto found = m_controllers.find(std::string(name));
        return found != m_controllers.end() ? found->second : nullptr;
    }

    const Animator* AnimationBindingResolver::tryGetController(
        std::string_view name) const
    {
        const auto found = m_controllers.find(std::string(name));
        return found != m_controllers.end() ? found->second : nullptr;
    }

    void AnimationBindingResolver::setTargets(
        const std::vector<AnimationBindingTarget>& targetsValue)
    {
        m_targets = targetsValue;
    }

    void AnimationBindingResolver::addTarget(
        const AnimationBindingTarget& target)
    {
        if (target.targetName.empty() || target.controllerName.empty())
        {
            return;
        }

        m_targets.push_back(target);
    }

    void AnimationBindingResolver::clearTargets()
    {
        m_targets.clear();
    }

    const std::vector<AnimationBindingTarget>&
        AnimationBindingResolver::targets() const
    {
        return m_targets;
    }

    std::vector<AnimationBindingResolutionResult>
        AnimationBindingResolver::resolveAll(
            Composition::PageComposer& composer) const
    {
        std::vector<AnimationBindingResolutionResult> results;
        results.reserve(m_targets.size());

        for (const AnimationBindingTarget& target : m_targets)
        {
            results.push_back(resolveTarget(composer, target));
        }

        return results;
    }

    AnimationBindingResolutionResult AnimationBindingResolver::resolveTarget(
        Composition::PageComposer& composer,
        const AnimationBindingTarget& target) const
    {
        if (target.targetName.empty())
        {
            return makeSkippedResult(target, "Animation target name is empty.");
        }

        if (target.controllerName.empty())
        {
            return makeSkippedResult(
                target,
                "Animation target has no controller reference.");
        }

        const Animator* animator = tryGetController(target.controllerName);
        if (animator == nullptr)
        {
            return makeSkippedResult(
                target,
                "Animation controller reference could not be resolved.");
        }

        return placeResolvedFrame(composer, target, *animator);
    }

    void AnimationBindingResolver::updateBoundControllers(const TickEvent& event)
    {
        std::vector<Animator*> updatedControllers;
        updatedControllers.reserve(m_controllers.size());

        for (const auto& entry : m_controllers)
        {
            Animator* animator = entry.second;
            if (animator == nullptr)
            {
                continue;
            }

            const bool alreadyUpdated =
                std::find(
                    updatedControllers.begin(),
                    updatedControllers.end(),
                    animator) != updatedControllers.end();

            if (alreadyUpdated)
            {
                continue;
            }

            animator->update(event);
            updatedControllers.push_back(animator);
        }
    }

    std::vector<AnimationBindingFrameState>
        AnimationBindingResolver::captureFrameState() const
    {
        std::vector<AnimationBindingFrameState> states;
        states.reserve(m_targets.size());

        for (const AnimationBindingTarget& target : m_targets)
        {
            AnimationBindingFrameState state;
            state.targetName = target.targetName;
            state.controllerName = target.controllerName;

            const Animator* animator = tryGetController(target.controllerName);
            if (animator != nullptr)
            {
                state.resolved = true;
                state.frameIndex = animator->currentFrameIndex();
            }

            states.push_back(state);
        }

        return states;
    }

    bool AnimationBindingResolver::isValidFrameIndex(
        std::size_t frameIndex,
        std::size_t frameCount)
    {
        return frameCount > 0 && frameIndex < frameCount;
    }

    int AnimationBindingResolver::toPlacementFrameIndex(std::size_t frameIndex)
    {
        if (frameIndex > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        {
            return std::numeric_limits<int>::max();
        }

        return static_cast<int>(frameIndex);
    }

    AnimationBindingResolutionResult
        AnimationBindingResolver::makeSkippedResult(
            const AnimationBindingTarget& target,
            std::string message)
    {
        AnimationBindingResolutionResult result;
        result.targetName = target.targetName;
        result.controllerName = target.controllerName;
        result.resolved = false;
        result.placed = false;
        result.message = std::move(message);
        return result;
    }

    AnimationBindingResolutionResult
        AnimationBindingResolver::placeResolvedFrame(
            Composition::PageComposer& composer,
            const AnimationBindingTarget& target,
            const Animator& animator) const
    {
        AnimationBindingResolutionResult result;
        result.targetName = target.targetName;
        result.controllerName = target.controllerName;
        result.resolved = true;

        const std::size_t frameIndex = animator.currentFrameIndex();
        result.frameIndex = frameIndex;

        switch (target.kind)
        {
        case AnimationBindingTargetKind::SequenceAssetPlacement:
            if (target.assetName.empty())
            {
                result.message = "Sequence asset placement has no asset name.";
                return result;
            }

            composer.placeSource(
                Composition::ObjectSource::fromAssetFrame(
                    target.assetName,
                    toPlacementFrameIndex(frameIndex)),
                target.placement,
                target.policy);

            result.placed = true;
            result.message = "Placed sequence asset frame.";
            return result;

        case AnimationBindingTargetKind::FramePlaceholder:
        {
            if (target.registeredFrames == nullptr)
            {
                result.message = "Frame placeholder has no registered frame list.";
                return result;
            }

            if (!isValidFrameIndex(frameIndex, target.registeredFrames->size()))
            {
                result.message = "Frame placeholder resolved frame is out of range.";
                return result;
            }

            const TextObject& frame = (*target.registeredFrames)[frameIndex];

            result.placementBounds = composer.resolvePlacementBounds(
                target.placement,
                Size{ frame.getWidth(), frame.getHeight() });

            composer.placeSource(
                Composition::ObjectSource::fromRegisteredFrame(
                    *target.registeredFrames,
                    toPlacementFrameIndex(frameIndex)),
                target.placement,
                target.policy);

            result.placed = true;
            result.message = "Placed registered frame placeholder.";
            return result;
        }

        case AnimationBindingTargetKind::RegisteredFrameSequence:
        {
            if (target.textAssetSequence == nullptr)
            {
                result.message = "Animated text sequence target has no sequence.";
                return result;
            }

            if (!isValidFrameIndex(frameIndex, target.textAssetSequence->frameCount()))
            {
                result.message = "Animated text sequence frame is out of range.";
                return result;
            }

            const AnimatedTextAssetFrameBuildResult frameResult =
                target.textAssetSequence->buildTextObjectForFrame(frameIndex);

            if (!frameResult.success || !frameResult.hasObject())
            {
                result.message = frameResult.errorMessage.empty()
                    ? "Animated text sequence did not produce a loaded frame."
                    : frameResult.errorMessage;
                return result;
            }

            result.placementBounds = composer.resolvePlacementBounds(
                target.placement,
                Size{ frameResult.object.getWidth(), frameResult.object.getHeight() });

            composer.placeSource(
                Composition::ObjectSource::fromTextObject(frameResult.object),
                target.placement,
                target.policy);

            result.placed = true;
            result.message = "Placed animated text sequence frame.";
            return result;
        }

        case AnimationBindingTargetKind::XpSequencePlacement:
            if (target.xpSequence == nullptr || !target.xpSequence->isValid())
            {
                result.message = "XP sequence placement has no valid XP sequence.";
                return result;
            }

            composer.placeSource(
                Composition::ObjectSource::fromXpSequence(
                    *target.xpSequence,
                    toPlacementFrameIndex(frameIndex),
                    target.xpFrameOptions),
                target.placement,
                target.policy);

            result.placed = true;
            result.message = "Placed XP sequence frame.";
            return result;

        default:
            result.message = "Unknown animation binding target kind.";
            return result;
        }
    }
}
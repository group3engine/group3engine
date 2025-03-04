//
// Created by thomas on 18/02/25.
//

#include "Animation.hpp"

#include "Entity.hpp"
#include <utility>
std::vector<NodeAnimation> Animation::CalcNodeAnimation(float aTime, const Skin &aTargetSkin) {
    // modulus the time by the max time
    aTime = std::fmod(aTime, mMaxTime);
    // result vector
    std::vector<NodeAnimation> result;
    // reserve one pair for each channel
    result.reserve(mTargets.size());
    Transform transform = aTargetSkin.GetEntity(mChannels[0].targetIndex)->GetTransform();
    int lastTargetIndex = -1;
    int channelNumber = 0;
    // for each channel, get the animation. If it is the same target as last
    // time, add it to the transform. Otherwise, add the last transform to the
    // result and start a new transform
    for (auto channel : mChannels) {
        // get the sampler
        auto &sampler = mSamplers[channel.sampler];
        // get the sampler value - this is one of a translation, rotation, or
        // scale, stored as a vec4 xyz(w)
        glm::vec4 value = sampler.GetSamplerValue(aTime, channel.transformChannel);
        // is the target the same as last time? If not, we've finished sampling
        // channels for this target, so it can be added to the result
        if (static_cast<int>(channel.targetIndex) == lastTargetIndex) {
        } else {
            if (lastTargetIndex != -1) {
                result.emplace_back(aTargetSkin.GetEntity(lastTargetIndex), transform);
            }
            lastTargetIndex = channel.targetIndex;
            // we use the currrent transform of the object as default for if
            // there isn't a keyframe for a channel to keep the value. This
            // follows GLTF spec (I think)
            transform = aTargetSkin.GetEntity(mChannels[++channelNumber].targetIndex)->GetTransform();
        }
        switch (channel.transformChannel) {
        case TransformChannel::TRANSLATION:
            transform.translation = value;
            break;
        case TransformChannel::ROTATION:
            transform.rotation = {value.x, value.y, value.z, value.w};
            break;
        case TransformChannel::SCALE:
            transform.scale = value;
            break;
        default:
            assert(false);
        }
    }
    // add the last transform
    if (lastTargetIndex != -1) {
        result.emplace_back(aTargetSkin.GetEntity(lastTargetIndex), transform);
    }
    return result;
}
glm::vec4 Sampler::GetSamplerValue(float aTime,
                                     TransformChannel aTransformChannel) {
    // edge cases
    {
        // if there are no keyframes, return the identity
        if (keyframes.empty()) {
            return {0, 0, 0, 1};
        }
        // if it is before the first keyframe, return the first keyframe
        if (aTime < keyframes.front().time) {
            return keyframes.front().value;
        }
        // if it is after the last keyframe, return the last keyframe
        if (aTime > keyframes.back().time) {
            return keyframes.back().value;
        }
    }
    // find the two keyframes that the time is between
    auto it = std::lower_bound(
        keyframes.begin(), keyframes.end(), aTime,
        [](const auto &frame, auto time) { return frame.time < time; });

    auto keyframe1 = std::prev(it);
    auto keyframe2 = std::next(keyframe1);
    if (interpolation == Interpolation::STEP) {
        return keyframe1->value;
    }

    // interpolate between the two keyframes
    // if this is a rotation, use spherical linear interpolation
    if (aTransformChannel == TransformChannel::ROTATION) {
        return Keyframe::Slerp(*keyframe1, *keyframe2, aTime);
    }
    // otherwise, use linear interpolation
    return Keyframe::Lerp(*keyframe1, *keyframe2, aTime);
}
glm::vec4 Keyframe::Lerp(Keyframe const &a, Keyframe const &b, float time) {
    float t = (time - a.time) / (b.time - a.time);
    return a.value * (1 - t) + b.value * t;
}
glm::vec4 Keyframe::Slerp(Keyframe const &a, Keyframe const &b, float time) {
    float t = (time - a.time) / (b.time - a.time);
    glm::quat q = glm::normalize(
        glm::slerp(glm::quat{a.value.x, a.value.y, a.value.z, a.value.w},
                   glm::quat{b.value.x, b.value.y, b.value.z, b.value.w}, t));
    return {q.x, q.y, q.z, q.w};
}
void Animation::SetName(std::string aName) { mName = std::move(aName); }
Sampler *Animation::    GetSampler(size_t i) { return &mSamplers[i]; }
float Animation::GetMaxTime() const { return mMaxTime; }
void Animation::AddChannel(Channel &aChannel) {
    mTargets.insert(aChannel.target);
    mChannels.push_back(aChannel);
    // sort the channels by target
    // they need to be sorted by target so that we can easily combine the channels for a target
    // stop and think before you remove this line
    std::sort(
        mChannels.begin(), mChannels.end(),
        [](const Channel &a, const Channel &b) { return *a.target < *b.target; });
}
void Animation::DetargetAnimation(std::unordered_map<Entity *, size_t> &aJointMap) {
    // for each channel, set the target index to the joint index
    for (auto &channel : mChannels) {
            channel.targetIndex = aJointMap[channel.target];
    }
}

std::vector<NodeAnimation> NodeAnimation::BlendAnimations(
    const std::vector<NodeAnimation> &aLeftAnimation,
    const std::vector<NodeAnimation> &aRightAnimation,
    float t) {
    std::vector<NodeAnimation> blendedAnimation;
    size_t leftCounter, rightCounter;
    leftCounter = rightCounter = 0;
    // we can assume that the two animation vectors have arrived to us sorted by
    // entity pointer, and use this to inform the merge. If one of the two
    // animations is missing an entity, we will assume an identity
    // transformation
    while (leftCounter < aLeftAnimation.size() ||
           rightCounter < aRightAnimation.size()) {
        // first case: no left animation, but right animation
        // if rightcounter is out of range this is triggered, or if
        // rightanimation pointer is greater than leftanimation pointer if
        // leftcounter is out of range this can't be triggered
        if (rightCounter >= aRightAnimation.size() ||
            (leftCounter < aLeftAnimation.size() &&
             *aLeftAnimation[leftCounter].target <
                 *aRightAnimation[rightCounter].target)) {
            Transform rightTransform{};
            rightTransform.translation = {1, 1, 1};
            rightTransform.rotation = {0, 0, 0, 1};
            rightTransform.scale = {1, 1, 1};
            Transform leftTransform = aLeftAnimation[leftCounter].transform;
            Transform resultingTransform =
                leftTransform.Interpolate(rightTransform, t);
            blendedAnimation.emplace_back(aLeftAnimation[leftCounter].target,
                                          resultingTransform);
            leftCounter++;
        }
        // second case, same as first but swap right with left - no right
        // animation, but left animation if leftcounter is out of range this is
        // triggered, or if leftanimation pointer is greater than rightanimation
        // pointer if rightcounter is out of range this can't be triggered
        else if (leftCounter >= aLeftAnimation.size() ||
                 (rightCounter < aRightAnimation.size() &&
                  *aRightAnimation[rightCounter].target <
                      *aLeftAnimation[leftCounter].target)) {
            Transform leftTransform{};
            leftTransform.translation = {1, 1, 1};
            leftTransform.rotation = {0, 0, 0, 1};
            leftTransform.scale = {1, 1, 1};
            Transform rightTransform = aRightAnimation[rightCounter].transform;
            Transform resultingTransform =
                leftTransform.Interpolate(rightTransform, t);
            blendedAnimation.emplace_back(aRightAnimation[rightCounter].target,
                                          resultingTransform);

            rightCounter++;
        }
        // final, base, most likely case: both have the animation. Simply blend
        // and increment both counters
        else if (*aLeftAnimation[leftCounter].target ==
                 *aRightAnimation[rightCounter].target) {
            Transform leftTransform = aLeftAnimation[leftCounter].transform;
            Transform rightTransform = aRightAnimation[rightCounter].transform;
            Transform resultingTransform =
                leftTransform.Interpolate(rightTransform, t);
            blendedAnimation.emplace_back(aRightAnimation[rightCounter].target,
                                          resultingTransform);

            leftCounter++;
            rightCounter++;
        }
    }

    return blendedAnimation;
}
//
// Created by thomas on 18/02/25.
//

#include "Animation.hpp"

#include "Entity.hpp"
#include <utility>
std::vector<NodeAnimation> Animation::GetAnimation(float aTime) {
    // modulus the time by the max time
    aTime = std::fmod(aTime, mMaxTime);
    // result vector
    std::vector<NodeAnimation> result;
    // reserve one pair for each channel
    result.reserve(mTargets.size());
    vk::Transform transform = mChannels[0].target->GetTransform();
    vk::Entity *lastTarget = nullptr;
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
        if (channel.target == lastTarget) {
        } else {
            if (lastTarget != nullptr) {
                result.emplace_back(lastTarget, transform);
            }
            lastTarget = channel.target;
            // we use the currrent transform of the object as default for if
            // there isn't a keyframe for a channel to keep the value. This
            // follows GLTF spec (I think)
            transform = mChannels[++channelNumber].target->GetTransform();
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
    if (lastTarget != nullptr) {
        result.emplace_back(lastTarget, transform);
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
    std::sort(
        mChannels.begin(), mChannels.end(),
        [](const Channel &a, const Channel &b) { return a.target < b.target; });
}

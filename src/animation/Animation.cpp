//
// Created by thomas on 18/02/25.
//

#include "Animation.hpp"
std::vector<std::pair<vk::Entity *, vk::Transform>>
Animation::GetAnimation(float aTime) {
    // modulus the time by the max time
    aTime = std::fmod(aTime, mMaxTime);
    // result vector
    std::vector<std::pair<vk::Entity *, vk::Transform>> result;
    // reserve one pair for each channel
    result.reserve(mTargets.size());
    vk::Transform transform{};
    vk::Entity *lastTarget = nullptr;
    // for each channel, get the animation. If it is the same target as last
    // time, add it to the transform. Otherwise, add the last transform to the
    // result and start a new transform
    for (auto channel : mChannels) {
        // get the sampler
        auto &sampler = *channel.sampler;
        // get the sampler value
        glm::vec4 value =
            GetSamplerValue(sampler, aTime, TransformChannel::TRANSLATION);
        // is the target the same as last time?
        if (channel.target == lastTarget) {
        } else {
            if (lastTarget != nullptr) {
                result.emplace_back(lastTarget, transform);
            }
            lastTarget = channel.target;
            transform = {};
        }
        switch (channel.transformChannel) {
        case TransformChannel::TRANSLATION:
            transform.translation = value;
            break;
        case TransformChannel::ROTATION:
            transform.rotation = glm::quat(value);
            break;
        case TransformChannel::SCALE:
            transform.scale = value;
            break;
        }
    }
    // add the last transform
    if (lastTarget != nullptr) {
        result.emplace_back(lastTarget, transform);
    }
    return result;
}
glm::vec4 Animation::GetSamplerValue(Sampler &aSampler, float aTime,
                                     TransformChannel aTransformChannel) {
    // edge cases
    {
        // if there are no keyframes, return the identity
        if (aSampler.keyframes.empty()) {
            return {0, 0, 0, 1};
        }
        // if it is before the first keyframe, return the first keyframe
        if (aTime < aSampler.keyframes.front().time) {
            return aSampler.keyframes.front().value;
        }
        // if it is after the last keyframe, return the last keyframe
        if (aTime > aSampler.keyframes.back().time) {
            return aSampler.keyframes.back().value;
        }
    }
    // find the two keyframes that the time is between
    // TODO: binary search if this is slow
    auto keyframe1 = aSampler.keyframes.begin();
    auto keyframe2 = std::next(keyframe1);
    while (keyframe2->time < aTime) {
        keyframe1 = keyframe2;
        keyframe2 = std::next(keyframe1);
    }
    if (aSampler.interpolation == Interpolation::STEP) {
        return keyframe1->value;
    }
    // interpolate between the two keyframes
    // if this is a rotation, use spherical linear interpolation
    if (aTransformChannel == TransformChannel::ROTATION) {
        return Slerp(*keyframe1, *keyframe2, aTime);
    }
    // otherwise, use linear interpolation
    return Lerp(*keyframe1, *keyframe2, aTime);
}
glm::vec4 Animation::Lerp(Keyframe &a, Keyframe &b, float time) {
    float t = (time - a.time) / (b.time - a.time);
    return a.value * (1 - t) + b.value * t;
}
glm::vec4 Animation::Slerp(Keyframe &a, Keyframe &b, float time) {
    float t = (time - a.time) / (b.time - a.time);
    glm::quat q =
        glm::normalize(glm::slerp(glm::quat(a.value), glm::quat(b.value), t));
    return {q.x, q.y, q.z, q.w};
}

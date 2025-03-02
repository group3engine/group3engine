//
// Created by thomas on 18/02/25.
//

#ifndef GROUP3ENGINE_ANIMATION_HPP
#define GROUP3ENGINE_ANIMATION_HPP

#include "GLTFImportStructs.hpp"
#include <glm/vec4.hpp>
#include <set>
#include <vector>

// forward declaration of Entity
namespace vk {
class Entity;
}

enum class Interpolation {
    LINEAR,
    STEP,
    // I don't want to support cubic spline for now
    // TODO: support cubic spline
    CUBICSPLINE
};
struct Keyframe {
    float time;
    glm::vec4 value;
};

struct Sampler {
    Interpolation interpolation;
    std::vector<Keyframe> keyframes;
};

enum class TransformChannel { TRANSLATION, ROTATION, SCALE };
struct Channel {
    vk::Entity *target;
    size_t sampler;
    TransformChannel transformChannel;
};

struct NodeAnimation{
    vk::Entity *target;
    vk::Transform transform;
};

class Animation {
  public:
    // default constructor
    Animation() = default;
    // resize the channels
    void ResizeChannels(size_t aSize) { mChannels.reserve(aSize); }
    // add a channel to the animation
    void AddChannel(Channel &aChannel);
    // resize the samplers
    void ResizeSamplers(size_t aSize) { mSamplers.reserve(aSize); }
    // add a sampler to the animation
    void AddSampler(Sampler &aSampler) {
        mSamplers.push_back(aSampler);
        // sort the keyframes by time
        std::sort(mSamplers.back().keyframes.begin(),
                  mSamplers.back().keyframes.end(),
                  [](const Keyframe &a, const Keyframe &b) {
                      return a.time < b.time;
                  });
        // update the max time
        mMaxTime = std::max(mMaxTime, mSamplers.back().keyframes.back().time);
    }
    // get the animation at a given time
    std::vector<NodeAnimation>
    GetAnimation(float aTime);

    void SetName(std::string aName);
    [[nodiscard]] std::string GetName() const { return mName; }

    Sampler* GetSampler(size_t i);

    [[nodiscard]] float GetMaxTime() const;

  private:
    // list of channels
    std::vector<Channel> mChannels;
    // list of samplers
    std::vector<Sampler> mSamplers;
    // set of target entities
    std::set<vk::Entity *> mTargets;
    // the highest time in the animation
    float mMaxTime = 0;
    // function to get the value of a sampler at a given time
    glm::vec4 GetSamplerValue(Sampler &aSampler, float aTime,
                              TransformChannel aTransformChannel);
    // function to interpolate between two keyframes
    glm::vec4 Lerp(Keyframe &a, Keyframe &b, float time);
    // function to spherical linear interpolate between two quaternions
    // (keyframes)
    glm::vec4 Slerp(Keyframe &a, Keyframe &b, float time);
    std::string mName;
};

#endif // GROUP3ENGINE_ANIMATION_HPP

//
// Created by thomas on 20/02/25.
//

#ifndef GROUP3ENGINE_ANIMATOR_HPP
#define GROUP3ENGINE_ANIMATOR_HPP

#include "Animation.hpp"
#include "Buffer.hpp"
#include "Skin.hpp"
#include <vulkan/vulkan.h>

#include <utility>
struct AnimationSample {
    float time;
    float weight;
};

class Animator {
  public:
    // constructor
    Animator(vk::Context *aContext, Skin *aSkin);
    // destructor
    ~Animator();
    // add an animation to the animator
    void AddAnimation(Animation *aAnimation) {
        mAnimations.push_back(aAnimation);
    }
    // set the animations
    void SetAnimations(std::vector<Animation *> aAnimations) {
        mAnimations = std::move(aAnimations);
    }
    // update the animator
    void Update(float aDeltaTime);
    // bind the descriptor set (for use when rendering)
    void BindDescriptorSet(VkCommandBuffer aCmdBuff,
                           VkPipelineLayout aPipelineLayout, int aSet);

  private:
    // update the joint buffer
    void UpdateJointBuffer();
    // update the animation samples
    void UpdateAnimationSamples(float aDeltaTime);
    // update the joints transform based on the animation samples
    void UpdateJointsTransform();

  public:
  private:
    // context
    vk::Context *mContext{};
    // descriptor set layout for the joint matrices
    VkDescriptorSetLayout mDescriptorSetLayout{};
    // descriptor set for the joint matrices
    VkDescriptorSet mDescriptorSet{};
    // descriptor pool for the joint matrices
    VkDescriptorPool mDescriptorPool{};
    // buffer for the joint matrices
    vk::Buffer mJointBuffer;

    // the skin that the animator is animating
    Skin *mSkin{};
    // a list of animations by reference
    std::vector<Animation *> mAnimations = {};

    // a map of animations (by index) to a sample
    std::unordered_map<int, AnimationSample> mAnimationSamples;
};

#endif // GROUP3ENGINE_ANIMATOR_HPP

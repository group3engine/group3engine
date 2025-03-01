//
// Created by thomas on 20/02/25.
//

#include "Animator.hpp"

#include "Entity.hpp"
#include "Utils.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include <cmath>
#include <glm/gtx/matrix_decompose.hpp>

Animator::~Animator() {
    // free the vulkan resources
    vkDestroyDescriptorSetLayout(mContext->device, mDescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorPool(mContext->device, mDescriptorPool, nullptr);
    // destroy the buffer
    mJointBuffer.Destroy();
}
void Animator::BindDescriptorSet(VkCommandBuffer aCmdBuff,
                                 VkPipelineLayout aPipelineLayout, int aSet) {
    // bind the descriptor set
    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            aPipelineLayout, aSet, 1, &mDescriptorSet, 0,
                            nullptr);
}
void Animator::Update(float aDeltaTime, vk::Entity *aMesh) {
    // update the animation samples
    UpdateAnimationSamples(aDeltaTime);
    // update the joints transform
    UpdateJointsTransform();
    // update the joint buffer
    UpdateJointBuffer(aMesh);
}
Animator::Animator(vk::Context *aContext, Skin *aSkin)
    : mContext(aContext), mSkin(aSkin) {
    // create the descriptor set layout
    mDescriptorSetLayout = vk::CreateDescriptorSetLayout(
        *mContext, {
                       {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                        VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                   });
    // create the descriptor pool
    vk::CreateDescriptorPool(*mContext, 1, 1, mDescriptorPool);
    // allocate the descriptor set
    vk::AllocateDescriptorSet(*mContext, mDescriptorPool, mDescriptorSetLayout,
                              1, mDescriptorSet);
    // create the joint buffer
    mJointBuffer = vk::CreateBuffer(
        "JointBuffer", *mContext, sizeof(glm::mat4) * mSkin->GetJoints().size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    // make the descriptor set point to the joint buffer
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = mJointBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(glm::mat4) * mSkin->GetJoints().size();
    vk::UpdateDescriptorSet(*mContext, 0, bufferInfo, mDescriptorSet,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}
void Animator::UpdateJointBuffer(vk::Entity *aMesh) {
    // get the joints from the skin
    auto joints = mSkin->GetJointMatrices(aMesh);
    // for each joint matrix, decompose it into its components
    for (auto &joint : joints) {
        // get the translation, rotation and scale
        glm::vec3 translation, scale;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(joint, scale, rotation, translation, skew, perspective);
        // debug output
        std::cout << "Translation: " << glm::to_string(translation)
                  << std::endl;
        std::cout << "Rotation: " << glm::to_string(rotation) << std::endl;
        std::cout << "Scale: " << glm::to_string(scale) << std::endl;
    }
    // upload the joints to the buffer
    mJointBuffer.WriteToBuffer(joints.data(),
                               sizeof(glm::mat4) * joints.size());
}
void Animator::UpdateAnimationSamples(float aDeltaTime) {
    // TODO: make an animation system
    if (mAnimations.empty()) {
        return;
    }
    // for each animation, set the weight to 0 and time to 0
    for (auto &sample : mAnimationSamples) {
        if (sample.first != mActiveAnimation &&
            sample.first != mLastAnimation) {
            sample.second.weight = 0;
            sample.second.time = 0;
        }
    }
    // get the blending percentages between the last and the current
    float pastWeight, currentWeight;
    pastWeight = 0.f;
    currentWeight = 1.f;
    // work out the interpolation multipliers (don't if we are in the cases
    // where we don't need to blend - no total blend time or no new animation)
    if (mTotalBlendTime > 0.01f && mLastAnimation != mActiveAnimation) {
        mCurrentBlendingTime += aDeltaTime;
        if (mCurrentBlendingTime > mTotalBlendTime) {
            mTotalBlendTime = 0.f;
        } else {
            currentWeight = mCurrentBlendingTime / mTotalBlendTime;
            pastWeight = 1.f - currentWeight;
        }
    }

    if (mActiveAnimation != -1) {
        mAnimationSamples[mActiveAnimation] = {
            (std::fmod(mAnimationSamples[mActiveAnimation].time +
                           aDeltaTime * mAnimationTimeScale,
                       mAnimations[mActiveAnimation]->GetMaxTime())),
            currentWeight};
    }
    if (mLastAnimation != -1) {
        mAnimationSamples[mLastAnimation] = {
            (std::fmod(mAnimationSamples[mLastAnimation].time +
                           aDeltaTime * mAnimationTimeScale,
                       mAnimations[mLastAnimation]->GetMaxTime())),
            pastWeight};
    }
}
void Animator::UpdateJointsTransform() {
    // TODO: make this blend the animations
    // For now, I will simply get the first animation and apply it to the joints
    if (mActiveAnimation == -1) {
    } else {
        auto currentAnimation = mAnimationSamples[mActiveAnimation];
        auto currentAnimationData =
            mAnimations[mActiveAnimation]->GetAnimation(currentAnimation.time);
        auto animationData = currentAnimationData;
        // If we are blending
        if (mTotalBlendTime > 0.01f) {
            auto pastAnimation = mAnimationSamples[mLastAnimation];
            auto pastAnimationData =
                mAnimations[mLastAnimation]->GetAnimation(pastAnimation.time);
            animationData =
                BlendAnimations(pastAnimationData, currentAnimationData,
                                currentAnimation.weight);
        }
        // for each entity in the animation data, update its transform
        for (auto &data : animationData) {
            auto entity = data.first;
            auto transform = data.second;
            entity->SetJointTransform(transform.getMatrix());
        }
    }
}
void Animator::SetActiveAnimation(const std::string &aName) {
    // for each animation, check if the name matches the string
    // if it does, set it as the active animation
    mActiveAnimation = -1;
    mTotalBlendTime = 0.f;
    for (size_t i = 0; i < mAnimations.size(); i++) {
        if (aName == mAnimations[i]->GetName()) {
            mCurrentAnimationName = aName;
            mActiveAnimation = static_cast<int>(i);
            return;
        }
    }
}

void Animator::SetActiveAnimation(const std::string &aName, float blendTime) {
    if (aName != mCurrentAnimationName && mActiveAnimation != -1) {
        mTotalBlendTime = blendTime;
        mLastAnimation = mActiveAnimation;
        mCurrentBlendingTime = 0.f;
    }
    for (size_t i = 0; i < mAnimations.size(); i++) {
        if (aName == mAnimations[i]->GetName()) {
            mCurrentAnimationName = aName;
            mActiveAnimation = static_cast<int>(i);
            return;
        }
    }
}
std::vector<std::pair<vk::Entity *, vk::Transform>> Animator::BlendAnimations(
    std::vector<std::pair<vk::Entity *, vk::Transform>> aLeftAnimation,
    std::vector<std::pair<vk::Entity *, vk::Transform>> aRightAnimation,
    float t) {
    std::vector<std::pair<vk::Entity *, vk::Transform>> blendedAnimation;
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
             aLeftAnimation[leftCounter].first <
                 aRightAnimation[rightCounter].first)) {
            vk::Transform rightTransform{};
            rightTransform.translation = {1, 1, 1};
            rightTransform.rotation = {0, 0, 0, 1};
            rightTransform.scale = {1, 1, 1};
            vk::Transform leftTransform = aLeftAnimation[leftCounter].second;
            vk::Transform resultingTransform =
                leftTransform.Interpolate(rightTransform, t);
            blendedAnimation.emplace_back(aLeftAnimation[leftCounter].first,
                                          resultingTransform);
            leftCounter++;
        }
        // second case, same as first but swap right with left - no right
        // animation, but left animation if leftcounter is out of range this is
        // triggered, or if leftanimation pointer is greater than rightanimation
        // pointer if rightcounter is out of range this can't be triggered
        else if (leftCounter >= aLeftAnimation.size() ||
                 (rightCounter < aRightAnimation.size() &&
                  aRightAnimation[rightCounter].first <
                      aLeftAnimation[leftCounter].first)) {
            vk::Transform leftTransform{};
            leftTransform.translation = {1, 1, 1};
            leftTransform.rotation = {0, 0, 0, 1};
            leftTransform.scale = {1, 1, 1};
            vk::Transform rightTransform = aRightAnimation[rightCounter].second;
            vk::Transform resultingTransform =
                leftTransform.Interpolate(rightTransform, t);
            blendedAnimation.emplace_back(aRightAnimation[rightCounter].first,
                                          resultingTransform);

            rightCounter++;
        }
        // final, base, most likely case: both have the animation. Simply blend
        // and increment both counters
        else if (aLeftAnimation[leftCounter].first ==
                 aRightAnimation[rightCounter].first) {
            vk::Transform leftTransform = aLeftAnimation[leftCounter].second;
            vk::Transform rightTransform = aRightAnimation[rightCounter].second;
            vk::Transform resultingTransform =
                leftTransform.Interpolate(rightTransform, t);
            blendedAnimation.emplace_back(aRightAnimation[rightCounter].first,
                                          resultingTransform);

            leftCounter++;
            rightCounter++;
        }
    }

    return blendedAnimation;
}
void Animator::SetTimeScale(float aTimeScale) {
    mAnimationTimeScale = aTimeScale;
}

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
void Animator::Update(double aDeltaTime, Entity *aMesh) {
    // update the animation samples
    UpdateAnimationSamples(aDeltaTime);
    // update the joints transform
    UpdateJointsTransform();
    // update the joint buffer
    UpdateJointBuffer(aMesh);
}
Animator::Animator(Context *aContext, Skin *aSkin)
    : mContext(aContext), mSkin(aSkin) {
    // create the descriptor set layout
    mDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(
        *mContext, {
                       {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                        VK_SHADER_STAGE_VERTEX_BIT, nullptr},
                   });
    // create the descriptor pool
    vkutil::CreateDescriptorPool(*mContext, 1, 1, mDescriptorPool);
    // allocate the descriptor set
    vkutil::AllocateDescriptorSet(*mContext, mDescriptorPool, mDescriptorSetLayout,
                              1, mDescriptorSet);
    // create the joint buffer
    mJointBuffer = CreateBuffer(
        "JointBuffer", *mContext, sizeof(glm::mat4) * mSkin->GetJoints().size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    // make the descriptor set point to the joint buffer
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = mJointBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(glm::mat4) * mSkin->GetJoints().size();
    vkutil::UpdateDescriptorSet(*mContext, 0, bufferInfo, mDescriptorSet,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}
void Animator::UpdateJointBuffer(Entity *aMesh) {
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
    }
    // upload the joints to the buffer
    mJointBuffer.Update(*mContext, joints.data(),
                               sizeof(glm::mat4) * joints.size());
}
void Animator::UpdateAnimationSamples(double aDeltaTime) {
    if (mAnimations.empty()) {
        return;
    }
    // reduce the animation lock time
    mAnimationLockTimer -= aDeltaTime;

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
            currentWeight = static_cast<float>(mCurrentBlendingTime / static_cast<double>(mTotalBlendTime));
            pastWeight = 1.f - currentWeight;
        }
    }

    if (mActiveAnimation != -1) {
        if(mAnimationLockTimer > 0.0f){
            // if the lock timer is still active, then don't modulus the time, but still increment it
                mAnimationSamples[mActiveAnimation] = {
                        mAnimationSamples[mActiveAnimation].time +
                                   aDeltaTime * mAnimationTimeScale,
                        currentWeight};
        }
        else {
            mAnimationSamples[mActiveAnimation] = {
                (std::fmod(mAnimationSamples[mActiveAnimation].time +
                               aDeltaTime * mAnimationTimeScale,
                           mAnimations[mActiveAnimation]->GetMaxTime())),
                currentWeight};
        }
    }
    if (mLastAnimation != -1) {
        mAnimationSamples[mLastAnimation] = {
            mAnimationSamples[mLastAnimation].time +
                           aDeltaTime * mAnimationTimeScale,
            pastWeight};
    }
}
void Animator::UpdateJointsTransform() {
    // For now, I will simply get the first animation and apply it to the joints
    if (mActiveAnimation == -1) {
    } else {
        const AnimationSample &currentAnimation = mAnimationSamples[mActiveAnimation];
        const std::vector<NodeAnimation> &currentAnimationData =
            mAnimations[mActiveAnimation]->CalcNodeAnimation(
                static_cast<float>(currentAnimation.time), *mSkin);
        std::vector<NodeAnimation> animationData = currentAnimationData;
        // If we are blending
        if (mTotalBlendTime > 0.01f) {
            const AnimationSample &pastAnimation = mAnimationSamples[mLastAnimation];
            const std::vector<NodeAnimation> &pastAnimationData =
                mAnimations[mLastAnimation]->CalcNodeAnimation(
                    static_cast<float>(pastAnimation.time), *mSkin);
            animationData =
                NodeAnimation::BlendAnimations(pastAnimationData, currentAnimationData, currentAnimation.weight);
        }
        // for each entity in the animation data, update its transform
        for (auto &data : animationData) {
            auto entity = data.target;
            auto transform = data.transform;
            entity->SetTransform(transform);
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

void Animator::SetActiveAnimation(const std::string &aName, float blendTime, bool lockForFirstLoop = false) {
    // if the lock timer is still active, then don't change the animation
    if (mAnimationLockTimer > 0.0f) {
            return;
    }
    if (aName != mCurrentAnimationName && mActiveAnimation != -1) {
        mTotalBlendTime = blendTime;
        mLastAnimation = mActiveAnimation;
        mCurrentBlendingTime = 0.f;
    }
    for (size_t i = 0; i < mAnimations.size(); i++) {
        if (aName == mAnimations[i]->GetName()) {
            mCurrentAnimationName = aName;
            // if we are locking for the first loop, and the animation is changed, then set the lock timer
            if(mActiveAnimation != static_cast<int>(i) && lockForFirstLoop){
                // set the lock timer to the animation duration
                mAnimationLockTimer = mAnimations[i]->GetMaxTime();
                // set the animations time to 0
                mAnimationSamples[static_cast<int>(i)].time = 0;
            }
            mActiveAnimation = static_cast<int>(i);
            return;
        }
    }
}

void Animator::SetTimeScale(float aTimeScale) {
    mAnimationTimeScale = aTimeScale;
}

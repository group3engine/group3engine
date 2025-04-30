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
    for (auto &buffer : mJointBuffers) {
        buffer.Destroy();
    }
}
void Animator::BindDescriptorSet(VkCommandBuffer aCmdBuff,
                                 VkPipelineLayout aPipelineLayout, int aSet, size_t aCurrentFrame) {
    // bind the descriptor set
    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            aPipelineLayout, aSet, 1, &mDescriptorSet[aCurrentFrame], 0,
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
    vkutil::CreateDescriptorPool(*mContext, vkutil::MAX_FRAMES_IN_FLIGHT, vkutil::MAX_FRAMES_IN_FLIGHT, mDescriptorPool);
    // resize the descriptor sets
    mDescriptorSet.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    // allocate the descriptor sets
    vkutil::AllocateDescriptorSets(*mContext, mDescriptorPool, mDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, mDescriptorSet);
    // create the joint buffers
    mJointBuffers.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    for (auto &buffer : mJointBuffers) {
        buffer = CreateBuffer("JointBuffer", *mContext, sizeof(glm::mat4) * mSkin->GetJoints().size(),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT);
    }
    //
    // make the descriptor sets point to the corresponding joint buffer
    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mJointBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(glm::mat4) * mSkin->GetJoints().size();
        vkutil::UpdateDescriptorSet(*mContext, 0, bufferInfo, mDescriptorSet[i],
                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
}
void Animator::UpdateJointBuffer(Entity *aMesh) {
    // get the joints from the skin
    mSkin->GetJointMatrices(aMesh, mJoints);
}

void Animator::UploadJointBuffer(VkCommandBuffer cmdBuff) {
    // upload the joints to the buffer
    mJointBuffers[vkutil::currentFrame].Upload(cmdBuff, mJoints.data(), sizeof(glm::mat4) * mJoints.size());
}

void Animator::UpdateAnimationSamples(double aDeltaTime) {
    if (mAnimations.empty()) {
        return;
    }
    // reduce the animation lock time
    mAnimationLockTimer -= std::abs(aDeltaTime);

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
        mCurrentBlendingTime += std::abs(aDeltaTime);
        if (mCurrentBlendingTime > mTotalBlendTime) {
            mTotalBlendTime = 0.f;
        } else {
            currentWeight = static_cast<float>(mCurrentBlendingTime / static_cast<double>(mTotalBlendTime));
            pastWeight = 1.f - currentWeight;
        }
    }

    if (mActiveAnimation != -1) {
        if(mAnimationLockTimer > 0.0f || !mActiveAnimationIsLooping){
            // if the lock timer is still active, then don't modulus the time, but still increment it
                mAnimationSamples[mActiveAnimation] = {
                        mAnimationSamples[mActiveAnimation].time +
                                   aDeltaTime * mAnimationTimeScale,
                        currentWeight};
        }
        else {
            float newTime = std::fmod(mAnimationSamples[mActiveAnimation].time +
                               aDeltaTime * mAnimationTimeScale,
                           mAnimations[mActiveAnimation]->GetMaxTime());
            if(aDeltaTime * mAnimationTimeScale < 0.0f && newTime < 0.0f){
                newTime = mAnimations[mActiveAnimation]->GetMaxTime() + newTime;
            }
            mAnimationSamples[mActiveAnimation] = {
                newTime,
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

void Animator::ResetActiveAnimation(const std::string &aName)
{
    // verify that the active animation matches the name
    if(aName != mCurrentAnimationName)
    {
        return;
    }
    mAnimationSamples[mActiveAnimation].time = 0;
}

void Animator::SetActiveAnimation(const std::string &aName, float blendTime, bool lockForFirstLoop, bool isLooping) {
    // if the lock timer is still active, then don't change the animation
    if (mAnimationLockTimer > 0.0f) {
            return;
    }
    if (aName != mCurrentAnimationName && mActiveAnimation != -1) {
        mTotalBlendTime = blendTime;
        mLastAnimation = mActiveAnimation;
        mLastAnimationIsLooping = mActiveAnimationIsLooping;
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
            mActiveAnimationIsLooping = isLooping;
            return;
        }
    }
}

void Animator::SetTimeScale(float aTimeScale) {
    mAnimationTimeScale = aTimeScale;
}

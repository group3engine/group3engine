//
// Created by thomas on 20/02/25.
//

#include "Animator.hpp"

#include "Utils.hpp"
#include <cmath>
#include "Entity.hpp"

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
void Animator::Update(float aDeltaTime) {
    // update the animation samples
    UpdateAnimationSamples(aDeltaTime);
    // update the joints transform
    UpdateJointsTransform();
    // update the joint buffer
    UpdateJointBuffer();
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
}
void Animator::UpdateJointBuffer() {
    // get the joints from the skin
    auto joints = mSkin->GetJoints();
    // upload the joints to the buffer
    mJointBuffer.WriteToBuffer(joints.data(),
                               sizeof(glm::mat4) * joints.size());
}
void Animator::UpdateAnimationSamples(float aDeltaTime) {
    // TODO: make an animation system. For now I will simply add deltaTime to
    // the time of the first animation, % by the duration and set the weight to
    // 1
    if (mAnimations.empty()) {
        return;
    }
    mAnimationSamples[0] = {(std::fmod(mAnimationSamples[0].time + aDeltaTime,
                                       mAnimations[0]->GetMaxTime())),
                            1};
}
void Animator::UpdateJointsTransform() {
    // TODO: make this blend the animations
    // For now, I will simply get the first animation and apply it to the joints
    auto animation = mAnimationSamples[0];
    auto animationData = mAnimations[0]->GetAnimation(animation.time);
    // for each entity in the animation data, update its transform
    for (auto &data : animationData) {
        auto entity = data.first;
        auto transform = data.second;
        entity->SetTransform(transform);
    }
}
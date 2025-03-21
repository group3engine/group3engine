//
// Created by thomas on 20/02/25.
//

#ifndef GROUP3ENGINE_ANIMATOR_HPP
#define GROUP3ENGINE_ANIMATOR_HPP

#include "Animation.hpp"
#include "Buffer.hpp"
#include "Skin.hpp"
#include "Volk.hpp"
#include <utility>
struct AnimationSample {
    double time = 0;
    float weight = 0;
};

class Animator {
  public:
    // constructor
    Animator(Context *aContext, Skin *aSkin);
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
    void Update(double aDeltaTime, Entity* aMesh);
    // bind the descriptor set (for use when rendering)
    void BindDescriptorSet(VkCommandBuffer aCmdBuff,
                           VkPipelineLayout aPipelineLayout, int aSet, size_t aCurrentFrame);

    void SetActiveAnimation(int index) { mActiveAnimation = index;
    }
    void SetActiveAnimation(const std::string&);
    void SetActiveAnimation(const std::string& aName, float blendTime, bool lockForFirstLoop);

    void SetTimeScale(float aTimeScale);

    void UploadJointBuffer(VkCommandBuffer cmdBuff);

  private:
    // update the joint buffer
    void UpdateJointBuffer(Entity* aMesh);
    // update the animation samples
    void UpdateAnimationSamples(double aDeltaTime);
    // update the joints transform based on the animation samples
    void UpdateJointsTransform();


  public:
  private:
    // context
    Context *mContext{};
    // descriptor set layout for the joint matrices
    VkDescriptorSetLayout mDescriptorSetLayout{};
    // descriptor set for the joint matrices
    std::vector<VkDescriptorSet> mDescriptorSet;
    // descriptor pool for the joint matrices
    VkDescriptorPool mDescriptorPool{};
    // buffer for the joint matrices
    std::vector<Buffer> mJointBuffers;

    int mActiveAnimation = -1;
    int mLastAnimation = -1;

    // the skin that the animator is animating
    Skin *mSkin{};
    // a list of animations by reference
    std::vector<Animation *> mAnimations = {};

    // a map of animations (by index) to a sample
    std::unordered_map<int, AnimationSample> mAnimationSamples;

    // the multiplier for an animations time
    float mAnimationTimeScale = 1.f;

    // the amount of time remaining until a new animation can be set
    float mAnimationLockTimer = 0.f;


    double mTotalBlendTime = 0.f;
    double mCurrentBlendingTime = 0.f;
    std::string mCurrentAnimationName{};

    std::vector<glm::mat4> mJoints;
};

#endif // GROUP3ENGINE_ANIMATOR_HPP

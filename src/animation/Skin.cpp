//
// Created by thomas on 20/02/25.
//
#include "Skin.hpp"
#include "Entity.hpp"
#include <glm/gtx/io.hpp>
#include <iostream>

void Skin::GetJointMatrices(Entity *aMesh, std::vector<glm::mat4> &aJointMatrices) const {
    aJointMatrices.clear();
    for (const auto &joint : mJoints) {
        aJointMatrices.push_back(joint.entity->GetWorldTransform() * joint.inverseBindMatrix);
    }
}
void Skin::AddJoint(Joint aJoint) {
    mJoints.push_back(aJoint);
}
// either detargets the animation or returns false because this isn't the right skin
bool Skin::DetargetAnimation(Animation *aAnimation, const std::vector<Entity *> &aEntititiesInAnimation) {
    // first we need to construct a map from entity pointer to joint index
    std::unordered_map<Entity *, size_t> entityToJointIndex;
    // for each entity, find the joint index
    // if there is no corresponding joint, return false
    for (auto *entity : aEntititiesInAnimation) {
        auto it = std::find_if(
            mJoints.begin(), mJoints.end(),
            [&entity](const auto &joint) { return joint.entity == entity; });
        if (it == mJoints.end()) {
            return false;
        } else {
            entityToJointIndex[entity] = it - mJoints.begin();
        }
    }
    // if we have reached this point, then we have found all the joints
    // detarget the animation - give the jointIndex to the channels
    aAnimation->DetargetAnimation(entityToJointIndex);
    return true;
}
Entity *Skin::GetEntity(size_t aIndex) const  {
    assert(aIndex < mJoints.size());
    return mJoints[aIndex].entity;
}

void Skin::ComputeRoot()
{
    // check if the root is already set
    if (mRoot != nullptr) {
        return;
    }
    // find the first parent of the first joint that is not a joint
    // this is the root of the skin
    auto *jointEntity = mJoints[0].entity;
    while (jointEntity->GetParent() != nullptr)
    {
        // check if the parent is a joint
        auto it = std::find_if(mJoints.begin(), mJoints.end(),
                               [jointEntity](const auto &aJoint) { return aJoint.entity == jointEntity->GetParent(); });
        if (it == mJoints.end())
        {
            mRoot = jointEntity->GetParent();
            return;
        }
        jointEntity = jointEntity->GetParent();
    }
    mRoot = jointEntity;
}

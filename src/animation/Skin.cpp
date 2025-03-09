//
// Created by thomas on 20/02/25.
//
#include "Skin.hpp"
#include "Entity.hpp"
#include <glm/gtx/io.hpp>
#include <iostream>

std::vector<glm::mat4> Skin::GetJointMatrices(Entity *aMesh) const {
    std::vector<glm::mat4> jointMatrices;
    jointMatrices.reserve(mJoints.size());
    for (const auto &joint : mJoints) {
        jointMatrices.push_back(joint.entity->getWorldTransform() * joint.inverseBindMatrix);
    }
    return jointMatrices;
}
void Skin::AddJoint(Joint aJoint) {
    mJoints.push_back(aJoint);
    // add the inverse bind matrix to the joint
    aJoint.entity->SetInverseBindMatrix(aJoint.inverseBindMatrix);
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

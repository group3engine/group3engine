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
        jointMatrices.push_back(glm::inverse(aMesh->getWorldTransform()) *
                                joint.entity->getWorldTransform() * joint.inverseBindMatrix);
    }
    return jointMatrices;
}
void Skin::AddJoint(Joint aJoint) {
    mJoints.push_back(aJoint);
    // add the inverse bind matrix to the joint
    aJoint.entity->SetInverseBindMatrix(aJoint.inverseBindMatrix);
}
// either detargets the animation or returns false because this isn't the right skin
bool Skin::DetargetAnimation(Animation *aAnimation, std::vector<Entity *> aEntititiesInAnimation) {
    // first we need to construct a map from entity pointer to joint index
    std::unordered_map<Entity *, size_t> entityToJointIndex;
    // for each entity, find the joint index
    // if there is no corresponding joint, return false
    for (auto const entity : aEntititiesInAnimation) {
        bool found = false;
        for (size_t i = 0; i < mJoints.size(); i++) {
            if (mJoints[i].entity == entity) {
                entityToJointIndex[entity] = i;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    // if we have reached this point, then we have found all the joints
    // detarget the animation - give the jointIndex to the channels
    aAnimation->DetargetAnimation(entityToJointIndex);
    return true;
}
Entity *Skin::GetEntity(size_t aIndex) const  {
    assert(aIndex < mJoints.size() && aIndex >= 0);
    return mJoints[aIndex].entity;
}

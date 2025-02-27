//
// Created by thomas on 20/02/25.
//
#include "Skin.hpp"
#include "Entity.hpp"
#include <iostream>
#include <glm/gtx/io.hpp>

std::vector<glm::mat4> Skin::GetJointMatrices(vk::Entity* aMesh) const {
    std::vector<glm::mat4> jointMatrices;
    jointMatrices.reserve(mJoints.size());
    for (const auto &joint : mJoints) {
        jointMatrices.push_back(glm::inverse(aMesh->getWorldTransform()) * joint.entity->getWorldTransform() * joint.inverseBindMatrix);
    }
    return jointMatrices;
}
void Skin::AddJoint(Joint aJoint) {
    mJoints.push_back(aJoint);
    // add the inverse bind matrix to the joint
    aJoint.entity->SetInverseBindMatrix(aJoint.inverseBindMatrix);
}

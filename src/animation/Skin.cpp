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
//        std::cout << joint.entity->GetName() << std::endl;
//        std::cout << glm::inverse(joint.entity->getSkinnedWorldTransform()) << std::endl;
//        std::cout << joint.inverseBindMatrix << std::endl;
//        std::cout << "Joint: " << joint.entity->GetName() << std::endl;
//        std::cout << joint.entity->getSkinnedWorldTransform() << std::endl;
        std::cout << joint.inverseBindMatrix << std::endl;
        jointMatrices.push_back(glm::inverse(aMesh->getWorldTransform()) * joint.entity->getSkinnedWorldTransform(nullptr) * joint.inverseBindMatrix);
    }
    return jointMatrices;
}
void Skin::AddJoint(Joint aJoint) {
    mJoints.push_back(aJoint);
    // add the inverse bind matrix to the joint
    aJoint.entity->SetInverseBindMatrix(aJoint.inverseBindMatrix);
}

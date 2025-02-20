//
// Created by thomas on 20/02/25.
//
#include "Skin.hpp"
#include "Entity.hpp"

std::vector<glm::mat4> Skin::GetJointMatrices() const {
    std::vector<glm::mat4> jointMatrices;
    jointMatrices.reserve(mJoints.size());
    for (const auto &joint : mJoints) {
        jointMatrices.push_back(joint.entity->getWorldTransform() *
                                joint.inverseBindMatrix);
    }
    return jointMatrices;
}

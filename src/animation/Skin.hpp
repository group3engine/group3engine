//
// Created by thomas on 18/02/25.
//

#ifndef GROUP3ENGINE_SKIN_HPP
#define GROUP3ENGINE_SKIN_HPP

#include "Entity.hpp"
struct Joint {
    vk::Entity *entity;
    glm::mat4 inverseBindMatrix;
};

class Skin {
  public:
    Skin() = default;
    void AddJoint(Joint aJoint) { mJoints.push_back(aJoint); }
    void SetJoints(std::vector<Joint> aJoints) { mJoints = std::move(aJoints); }
    [[nodiscard]] std::vector<Joint> GetJoints() const { return mJoints; }
    // function to get the joint matrices
    [[nodiscard]] std::vector<glm::mat4> GetJointMatrices() const {
        std::vector<glm::mat4> jointMatrices;
        jointMatrices.reserve(mJoints.size());
        for (const auto &joint : mJoints) {
            jointMatrices.push_back(joint.entity->getWorldTransform() *
                                    joint.inverseBindMatrix);
        }
        return jointMatrices;
    }

  private:
    // list of joints
    std::vector<Joint> mJoints;
};

#endif // GROUP3ENGINE_SKIN_HPP

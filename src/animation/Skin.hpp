//
// Created by thomas on 18/02/25.
//

#ifndef GROUP3ENGINE_SKIN_HPP
#define GROUP3ENGINE_SKIN_HPP

#include <glm/glm.hpp>

namespace vk
{
        class Entity;
}



struct Joint {
    vk::Entity *entity;
    glm::mat4 inverseBindMatrix;
};

class Skin {
  public:
    Skin() = default;
    void ResizeJoints(size_t aSize) { mJoints.reserve(aSize); }
    void AddJoint(Joint aJoint) { mJoints.push_back(aJoint); }
    void SetJoints(std::vector<Joint> aJoints) { mJoints = std::move(aJoints); }
    void SetRoot(vk::Entity *aRoot) { mRoot = aRoot; }

    [[nodiscard]] std::vector<Joint> GetJoints() const { return mJoints; }
    // function to get the joint matrices
    [[nodiscard]] std::vector<glm::mat4> GetJointMatrices() const;

    void SetName(char *aName) {mName = aName;}
    [[nodiscard]] std::string GetName() const { return mName; }

  private:
    // list of joints
    std::vector<Joint> mJoints;
    // root joint
    vk::Entity *mRoot;
    // name of the skin
        std::string mName;
};

#endif // GROUP3ENGINE_SKIN_HPP

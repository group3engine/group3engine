//
// Created by thomas on 18/02/25.
//

#ifndef GROUP3ENGINE_SKIN_HPP
#define GROUP3ENGINE_SKIN_HPP

#include "Animation.hpp"
#include <glm/glm.hpp>
#include <string>

class Entity;

struct Joint {
    Entity *entity = nullptr;
    glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
};

class Skin {
  public:
    Skin() = default;
    void ResizeJoints(size_t aSize) { mJoints.reserve(aSize); }
    void AddJoint(Joint aJoint);
    void SetJoints(std::vector<Joint> aJoints) { mJoints = std::move(aJoints); }
    void SetRoot(Entity *aRoot) { mRoot = aRoot; }

    [[nodiscard]] std::vector<Joint> GetJoints() const { return mJoints; }
    // function to get the joint matrices
    [[nodiscard]] std::vector<glm::mat4> GetJointMatrices(Entity* aMesh) const;

    void SetName(char *aName) { mName = aName; }
    [[nodiscard]] std::string GetName() const { return mName; }


    bool DetargetAnimation(Animation *aAnimation, std::vector<Entity *> aEntititiesInAnimation);

  private:
    // list of joints
    std::vector<Joint> mJoints;
    // root joint
    Entity *mRoot;
    // name of the skin
    std::string mName;
};

#endif // GROUP3ENGINE_SKIN_HPP

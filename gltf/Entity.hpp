//
// Created by thomas on 07/02/25.
//

#ifndef VULKANTIME_ENTITY_HPP
#define VULKANTIME_ENTITY_HPP
#include <utility>

#include "GLTFImportStructs.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "vulkan/vulkan.h"

class Entity {
  public:
    Entity(std::string aName, Entity *aParent, Mesh *aMesh, Transform aLocalTransform, VkPipelineLayout aPipelineLayout)
        : mName(std::move(aName)), mParent(aParent), mMesh(aMesh), mLocalTransform(aLocalTransform),
          mHasMesh(true), mPipelineLayout(aPipelineLayout) {}
    Entity(std::string aName, Entity *aParent, Mesh *aMesh, glm::mat4 aLocalTransform, VkPipelineLayout aPipelineLayout);

    explicit Entity(VkPipelineLayout aPipelineLayout) : mPipelineLayout(aPipelineLayout) {}

    void SetName(std::string aName) { mName = std::move(aName); }
    void SetParent(Entity *aParent);
    void AddChild(Entity *aChild) { children.push_back(aChild); }
    void AddMesh(Mesh *mesh) {
        mMesh = mesh;
        mHasMesh = true;
    }
    void SetTransform(Transform aTransform) { mLocalTransform = aTransform; }
    void SetTransform(glm::mat4 aTransform);

    ~Entity() = default;

    glm::mat4 getWorldTransform();

    glm::mat4 getLocalTransform();

    void RecordDraw(VkCommandBuffer aCmdBuff);

  private:
    std::string mName{};
    Entity *mParent = nullptr;
    std::vector<Entity *> children;
    Mesh *mMesh = nullptr;
    Transform mLocalTransform{};
    bool mHasMesh = false;

    VkPipelineLayout mPipelineLayout;
    //    bool mHasPhysics = false;
};

#endif // VULKANTIME_ENTITY_HPP

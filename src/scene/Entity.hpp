//
// Created by thomas on 07/02/25.
//

#ifndef VULKANTIME_ENTITY_HPP
#define VULKANTIME_ENTITY_HPP
#include "Volk.hpp"
#include <utility>

#include "Animator.hpp"
#include "GLTFImportStructs.hpp"
#include "animation/Animator.hpp"
#include <glm/glm.hpp>

namespace vk {
class Entity {
   public:
    Entity(std::string aName, Entity *aParent, Mesh *aMesh,
           Transform aLocalTransform)
        : mName(std::move(aName)),
          mParent(aParent),
          mMesh(aMesh),
          mLocalTransform(aLocalTransform),
          mHasMesh(true){}
    Entity(std::string aName, Entity *aParent, Mesh *aMesh,
           glm::mat4 aLocalTransform);

    Entity() = default;

    Entity(const Entity &) = default;
    Entity(Entity &&) = default;
    Entity &operator=(const Entity &) = default;
    Entity &operator=(Entity &&) = default;
    virtual ~Entity();

    void SetName(std::string aName) { mName = std::move(aName); }
    std::string GetName() const { return mName; }
    void SetParent(Entity *aParent);
    [[nodiscard]] Entity *GetParent() const { return mParent; }
    [[nodiscard]] std::vector<Entity *> GetChildren() const { return children; }
    void AddChild(Entity *aChild) { children.push_back(aChild); }
    void AddMesh(Mesh *mesh) {
        mMesh = mesh;
        mHasMesh = true;
    }
    void SetTransform(Transform aTransform) { mLocalTransform = aTransform; }
    void SetTransform(glm::mat4 aTransform);
    void SetJointTransform(glm::mat4 aJointTransform) {
        SetTransform(aJointTransform); }

    void SetInverseBindMatrix(glm::mat4 aInverseBindMatrix) {
        mInverseBindMatrix = aInverseBindMatrix;
    }

    [[nodiscard]] glm::mat4 getWorldTransform() const;

    [[nodiscard]] glm::mat4
    getSkinnedWorldTransform(Entity const *aRoot) const;

    [[nodiscard]]

    glm::mat4 getLocalTransform();

    void RecordDrawOpaque(VkCommandBuffer aCmdBuff,
                          VkPipelineLayout aPipelineLayout);

    void RecordDrawShadow(VkCommandBuffer aCmdBuff,
                          VkPipelineLayout aPipelineLayout) const;

    void RecordDrawCutout(VkCommandBuffer aCmdBuff,
                          VkPipelineLayout aPipelineLayout);
    // move an animator to the entity
    void SetAnimator(Animator *aAnimator);

    void RecordDrawSkinned(VkCommandBuffer aCmdBuff,
                           VkPipelineLayout aPipeLayout);

    virtual void Update(double deltaTime);

    Transform GetTransform();

  protected:
    virtual void LateUpdate(){}
    virtual void Awake(){}

   private:
    std::string mName{};
    Entity *mParent = nullptr;
    std::vector<Entity *> children;
    Mesh *mMesh = nullptr;
    Transform mLocalTransform{};
    bool mHasMesh = false;
    Animator *mAnimator = nullptr;
    int frameNumber = 0;

    //    bool mHasPhysics = false;
    glm::mat4 mInverseBindMatrix = glm::mat4(1.0f);
    glm::mat4 mAnimationTransform = glm::mat4(1.0f);
};
}  // namespace vk
#endif  // VULKANTIME_ENTITY_HPP

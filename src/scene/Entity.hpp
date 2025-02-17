//
// Created by thomas on 07/02/25.
//

#ifndef VULKANTIME_ENTITY_HPP
#define VULKANTIME_ENTITY_HPP
#include "Volk.hpp"
#include <utility>

#include "GLTFImportStructs.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/matrix_decompose.hpp"

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
    virtual ~Entity() = default;

    void SetName(std::string aName) { mName = std::move(aName); }
    void SetParent(Entity *aParent);
    void AddChild(Entity *aChild) { children.push_back(aChild); }
    void AddMesh(Mesh *mesh) {
        mMesh = mesh;
        mHasMesh = true;
    }
    void SetTransform(Transform aTransform) { mLocalTransform = aTransform; }
    void SetTransform(glm::mat4 aTransform);

    [[nodiscard]] glm::mat4 getWorldTransform() const;

    glm::mat4 getLocalTransform();

    void RecordDrawOpaque(VkCommandBuffer aCmdBuff,
                          VkPipelineLayout aPipelineLayout);

    void RecordDrawShadow(VkCommandBuffer aCmdBuff,
                          VkPipelineLayout aPipelineLayout) const;

    void RecordDrawCutout(VkCommandBuffer aCmdBuff,
                          VkPipelineLayout aPipelineLayout);

   protected:
    virtual void Update(){}
    virtual void LateUpdate(){}
    virtual void Awake(){}

   private:
    std::string mName{};
    Entity *mParent = nullptr;
    std::vector<Entity *> children;
    Mesh *mMesh = nullptr;
    Transform mLocalTransform{};
    bool mHasMesh = false;

    //    bool mHasPhysics = false;
};
}  // namespace vk
#endif  // VULKANTIME_ENTITY_HPP

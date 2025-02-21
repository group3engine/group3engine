#ifndef SCENE_ENTITY_HPP
#define SCENE_ENTITY_HPP

#include <utility>

#include <glm/fwd.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "GLTFImportStructs.hpp"
#include "RigidBody.hpp"

class Entity {
  public:
    Entity(std::string aName, Entity *aParent, Mesh *aMesh, Transform aLocalTransform)
        : mName(std::move(aName)), mParent(aParent), mMesh(aMesh), mLocalTransform(aLocalTransform),
          mHasMesh(true) {}
    Entity(std::string aName, Entity *aParent, Mesh *aMesh, glm::mat4 aLocalTransform);

    Entity() = default;
    // Delete copy constructors because of unique pointer to rigid body
    Entity(const Entity &) = delete;
    Entity &operator=(const Entity &) = delete;
    Entity(Entity &&) = default;
    Entity &operator=(Entity &&) = default;
    virtual ~Entity() = default;

    void SetName(std::string aName) { mName = std::move(aName); }

    void SetParent(Entity *aParent);

    void AddChild(Entity *aChild) { mChildren.push_back(aChild); }

    void AddMesh(Mesh *mesh) {
        mMesh = mesh;
        mHasMesh = true;
    }

    void AddRigidBody(std::unique_ptr<RigidBody> rigidBody) {
        mRigidBody = std::move(rigidBody);
        mHasRigidBody = true;
    }

    void SetTransform(Transform aTransform) { mLocalTransform = aTransform; }
    void SetTransform(glm::mat4 aTransform);

    [[nodiscard]] glm::mat4 getWorldTransform() const;

    glm::mat4 getLocalTransform();

    void RecordDrawOpaque(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout);

    void RecordDrawShadow(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout) const;

    void RecordDrawCutout(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout);

  protected:
    virtual void Update() {}
    virtual void LateUpdate() {}
    virtual void Awake() {}

  public:
    std::unique_ptr<RigidBody> mRigidBody;

    glm::vec3 mPosition{};
    bool mHasCharacter = false;

    std::string mName{};
  private:
    

    Entity *mParent = nullptr;
    std::vector<Entity *> mChildren;

    Mesh *mMesh = nullptr;

    Transform mLocalTransform{};

    bool mHasMesh = false;
    bool mHasRigidBody = false;
};
#endif // VULKANTIME_ENTITY_HPP

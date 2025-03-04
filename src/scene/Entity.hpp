#ifndef SCENE_ENTITY_HPP
#define SCENE_ENTITY_HPP

#include <utility>

#include <glm/glm.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "Animator.hpp"
#include "GLTFImportStructs.hpp"
#include "RigidBody.hpp"

#include <atomic>


class Entity {
  private:
    static std::atomic<uint32_t> kEntityCount;

  public:
    Entity(std::string aName, Entity *aParent, Mesh *aMesh,
           Transform aLocalTransform)
        : mName(std::move(aName)), mParent(aParent), mMesh(aMesh),
          mLocalTransform(aLocalTransform), mHasMesh(true) {}
    Entity(std::string aName, Entity *aParent, Mesh *aMesh,
           glm::mat4 aLocalTransform);

    Entity() = default;
    // Delete copy constructors because of unique pointer to rigid body
    Entity(const Entity &) = delete;
    Entity &operator=(const Entity &) = delete;
    Entity(Entity &&) = default;
    Entity &operator=(Entity &&) = default;
    virtual ~Entity();

    // equality operator
    bool operator==(const Entity &aOther) const {
        return mEntityID == aOther.mEntityID;
    }
    // greater than operator
    // the order is meaningless but stable
    bool operator>(const Entity &aOther) const {
        return mEntityID > aOther.mEntityID;
    }
    // less than operator
    // the order is meaningless but stable
    bool operator<(const Entity &aOther) const {
        return mEntityID < aOther.mEntityID;
    }

    void SetName(std::string aName) { mName = std::move(aName); }
    [[nodiscard]] const std::string &GetName() const { return mName; }

    void SetParent(Entity *aParent);

    [[nodiscard]] Entity *GetParent() const { return mParent; }
    [[nodiscard]] std::vector<Entity *> const &GetChildren() { return mChildren; }
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
    void SetJointTransform(glm::mat4 aJointTransform) {
        SetTransform(aJointTransform);
    }

    void SetInverseBindMatrix(glm::mat4 aInverseBindMatrix) {
        mInverseBindMatrix = aInverseBindMatrix;
    }

    [[nodiscard]] glm::mat4 getWorldTransform() const;

    [[nodiscard]] glm::mat4 getSkinnedWorldTransform(Entity const *aRoot) const;

    glm::mat4 getLocalTransform();

    void RecordDrawOpaque(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout);

    void RecordDrawShadow(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout) const;

    void RecordDrawCutout(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout);

    const Mesh *GetMesh() const { return mMesh; }

    // move an animator to the entity
    void SetAnimator(Animator *aAnimator);

    void RecordDrawSkinned(VkCommandBuffer aCmdBuff,
                           VkPipelineLayout aPipeLayout);

    virtual void Update(double deltaTime);

    Transform GetTransform();

  protected:
    virtual void LateUpdate() {}
    virtual void Awake() {}
    virtual void OnCollisionStart(Entity *aOther) {}
    virtual void OnCollisionStay(Entity *aOther) {}
    virtual void OnCollisionEnd(Entity *aOther) {}


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
    Animator *mAnimator = nullptr;
    size_t frameNumber = 0;
    glm::mat4 mInverseBindMatrix = glm::mat4(1.0f);
    glm::mat4 mAnimationTransform = glm::mat4(1.0f);

    bool mHasRigidBody = false;

    uint32_t mEntityID = kEntityCount++;
};
#endif // VULKANTIME_ENTITY_HPP

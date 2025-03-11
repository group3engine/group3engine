#ifndef SCENE_ENTITY_HPP
#define SCENE_ENTITY_HPP

#include <utility>

#include <glm/glm.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "Animator.hpp"
#include "GLTFImportStructs.hpp"
#include "RigidBody.hpp"
#include "spdlog/spdlog.h"

#include <atomic>


class Entity {
  private:
    static std::atomic<uint32_t> kEntityCount;

  public:
    // functions the user can call
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


    void SetTransform(Transform aTransform) { mLocalTransform = aTransform; SetPhysicsTransform();}
    void SetTransform(glm::mat4 aTransform);

    [[nodiscard]] glm::mat4 GetWorldTransform() const;

    [[nodiscard]] Transform GetWorldTransformComponents() const;




    [[nodiscard]] const Mesh *GetMesh() const { return mMesh; }



    void RecordDrawSkinned(VkCommandBuffer aCmdBuff,
                           VkPipelineLayout aPipeLayout);

    [[nodiscard]] Transform GetTransform();

    [[nodiscard]] bool IsCharacter() const;

    [[nodiscard]] bool HasAnimator() const { return static_cast<bool>(mAnimator); }

    // get a reference to the animator
    [[nodiscard]] Animator & GetAnimator(){return *mAnimator;}


    void AddTag(const std::string& aTag) { mTags.emplace_back(aTag); }
    [[nodiscard]] bool CompareTag(const std::string& aTag);

    void SetAsSensor() { mIsSensor = true; }
    [[nodiscard]] bool IsSensor() const { return mIsSensor; }

    void SetAsNotSolid() { mIsSolid = false; }
    [[nodiscard]] bool IsSolid() const { return mIsSolid; }

    void SetAsKinematic() { mIsKinematic = true; }
    [[nodiscard]] bool IsKinematic() const { return mIsKinematic; }

    void SetAsInvisible() { mIsVisible = false; }
    void SetAsVisible() { mIsVisible = true; }

  public:
    // the following functions are overridable by the user
    virtual void OnCollisionStart(Entity *aOther) {}

    virtual void OnCollisionStay(Entity *aOther) {}

    virtual void LateUpdate(double deltaTime) {}

    virtual void Awake() {}

    virtual void Update(double deltaTime) {}

  public:
    // functions used by the engine, the user should not call these
    void BaseUpdate(double deltaTime);
    void RecordDrawOpaque(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout);

    void RecordDrawShadow(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout) const;

    void RecordDrawCutout(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout);
    // move an animator to the entity
    void SetAnimator(Animator *aAnimator);

    void AddChild(Entity *aChild) { mChildren.push_back(aChild); }

    void AddMesh(Mesh *mesh) {
        mMesh = mesh;
        mHasMesh = true;
    }

    void AddRigidBody(std::unique_ptr<RigidBody> rigidBody) {
        mRigidBody = std::move(rigidBody);
        PhysicsManager::get().RegisterEntity(this, mRigidBody->mBodyId);
        mHasRigidBody = true;
    }

    [[nodiscard]] glm::mat4 getSkinnedWorldTransform(Entity const *aRoot) const;



  private:
    void SetPhysicsTransform();
    void RemoveChild(Entity *aChild);

  protected:


    std::vector<Entity *> mChildren;


    vector<std::string> mTags;

  public:
    std::unique_ptr<RigidBody> mRigidBody;

    glm::vec3 mCharacterPositionOffset{};
    bool mHasCharacter = false;

    std::string mName{};
  private:
    

    Entity *mParent = nullptr;

    Mesh *mMesh = nullptr;

    Transform mLocalTransform{};

    bool mHasMesh = false;
    Animator *mAnimator = nullptr;
    size_t frameNumber = 0;
    glm::mat4 mAnimationTransform = glm::mat4(1.0f);

    bool mHasRigidBody = false;

    uint32_t mEntityID = kEntityCount++;

    bool mIsSensor = false;

    bool mIsSolid = true;

    bool mIsVisible = true;

    bool mIsKinematic = false;

};
#endif // SCENE_ENTITY_HPP

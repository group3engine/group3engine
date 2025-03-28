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

class Scene;

enum class PhysicsType {
    STATIC,
    KINEMATIC,
    DYNAMIC
};

/// The base class for all entities in the scene. Custom entities all have this as their base class
class Entity {
  private:
    static std::atomic<uint32_t> kEntityCount;

  public:
    // functions the user can call

    /// Constructor with a name, parent (nullptr if none), mesh, and local transform as a Transform
    Entity(std::string aName, Entity *aParent, Mesh *aMesh,
           Transform aLocalTransform)
        : mName(std::move(aName)), mParent(aParent), mMesh(aMesh),
          mLocalTransform(aLocalTransform), mHasMesh(true) {mLocalTransform.UpdateMatrix(); UpdateChildrenTransform();}
    /// Constructor with a name, parent (nullptr if none), mesh, and local transform as a mat4
    Entity(std::string aName, Entity *aParent, Mesh *aMesh,
           glm::mat4 aLocalTransform);

    /// Default constructor
    Entity() = default;
    // Delete copy constructors because of unique pointer to rigid body
    Entity(const Entity &) = delete;
    Entity &operator=(const Entity &) = delete;
    Entity(Entity &&) = default;
    Entity &operator=(Entity &&) = default;

    // destructor
    virtual ~Entity();

    /// Equality operator, uses an entity ID to compare
    bool operator==(const Entity &aOther) const {
        return mEntityID == aOther.mEntityID;
    }
    // greater than operator
    // the order is meaningless but stable
    /// Greater than operator, uses an entity ID to compare. The order is stable at runtime, so can be used for sorting
    bool operator>(const Entity &aOther) const {
        return mEntityID > aOther.mEntityID;
    }
    // less than operator
    // the order is meaningless but stable
    /// Greater than operator, uses an entity ID to compare. The order is stable at runtime, so can be used for sorting
    bool operator<(const Entity &aOther) const {
        return mEntityID < aOther.mEntityID;
    }

    // getters and setters for the name
    /// Set the name of the entity. Does not need to be unique
    void SetName(std::string aName) { mName = std::move(aName); }
    /// Get the name of the entity
    [[nodiscard]] const std::string &GetName() const { return mName; }

    // getters and setters for the parent. The parent will automatically add this entity as a child
    /// Set the parent of the entity. The parent will automatically add this entity as a child
    void SetParent(Entity *aParent);
    /// Get a pointer to the parent of the entity
    [[nodiscard]] Entity *GetParent() const { return mParent; }

    // getter for the children
    /// Get a vector of pointers to the children of the entity
    [[nodiscard]] std::vector<Entity *> const &GetChildren() { return mChildren; }


    // setters for the transform, either as a Transform or a mat4. These also pass through the updated transform to the rigidbody
    /// Set the local transform of the entity as a #Transform. This will also update the physics transform
    void SetTransform(Transform aTransform);
    /// Set the local transform of the entity as a mat4. This will also update the physics transform
    void SetTransform(glm::mat4 aTransform);

    /// Get the local transform as a #Transform
    [[nodiscard]] Transform GetLocalTransform() const;

    /// Get the world transform as a glm::mat4
    [[nodiscard]] glm::mat4 GetWorldTransform() const;
    /// Get the world transform as a #Transform (more expensive than the mat4 version, this calls #GetWorldTransform and decomposes it)
    [[nodiscard]] Transform GetWorldTransformComponents() const;

    /// Get the #Mesh of the entity
    [[nodiscard]] Mesh *GetMesh() const { return mMesh; }

    /// Query if this entity is a character
    [[nodiscard]] bool IsCharacter() const;

    /// Query if this entity has an animator
    [[nodiscard]] bool HasAnimator() const { return static_cast<bool>(mAnimator); }

    /// Get a reference to the animator
    [[nodiscard]] Animator &GetAnimator(){return *mAnimator;}

    /// Get a reference to the rigidbody
    [[nodiscard]] RigidBody &GetRigidBody(){ return *mRigidBody; }


    /// Add a tag to the entity. Tags are also added from the GLTF file.
    void AddTag(const std::string& aTag) { mTags.emplace_back(aTag); }
    /// Query if the entity has a tag
    [[nodiscard]] bool CompareTag (const std::string& aTag) const;

    /// Query if the entity is a sensor (has physics but does not collide with other entities)
    [[nodiscard]] bool IsSensor() const { return mIsSensor; }

    /// Query if the entity is solid (collides with other entities)
    [[nodiscard]] bool IsSolid() const { return mIsSolid; }

    /// Query if the entity is invisible
    [[nodiscard]] bool IsVisible() const { return mIsVisible; }
    /// Set the entity as invisible
    void SetAsInvisible() { mIsVisible = false; }
    /// Set the entity as visible
    void SetAsVisible() { mIsVisible = true; }

    void SetPhysicsType(PhysicsType input_physics_type) { mPhysicsType = input_physics_type; }

    PhysicsType GetPhysicsType() const { return mPhysicsType; }

    /// Get the number of frames that have passed since the entity was created
    [[nodiscard]] size_t GetFrameNumber() const {return mFrameNumber;}

    /// Get the total time that has passed since the entity was created
    [[nodiscard]] double GetTotalTime() const {return mTotalTime;}

  public:
    // the following functions are overridable by the user
    // called on the first frame of a collision
    virtual void OnCollisionStart(Entity *aOther) {}

    // called on any frame except the first frame of a collision - note there is no function provided for the last frame of a collision
    virtual void OnCollisionStay(Entity *aOther) {}

    // called for each entity after update has been called on all entities
    virtual void LateUpdate(double deltaTime) {}

    // called after the entire scene has been loaded, once only
    virtual void Awake() {}

    /// called every frame, after physics has been updated
    virtual void Update(double deltaTime) {}

    virtual void UpdateUi(double deltaTime) {}

  public:
    // functions used by the engine, the user should not call these
    void BaseUpdate(double deltaTime);

    void RecordDrawOpaque(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout) const;

    void RecordDrawShadow(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout) const;

    void RecordDrawCutout(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout) const;

    void RecordDrawSkinned(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipeLayout) const;
    // move an animator to the entity
    void SetAnimator(Animator *aAnimator);

    void AddChild(Entity *aChild) { mChildren.push_back(aChild); }

    void SetAsCharacter() {mHasCharacter = true;}

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

    // set the entity as not solid
    void SetAsNotSolid() { mIsSolid = false; }
    // set the entity as a sensor
    void SetAsSensor() { mIsSensor = true; }
    // set the parent transform
    void SetParentTransform(glm::mat4 aParentTransform);

    // TODO: Make friend class with Scene
    void SetScene(Scene *scene) { mScene = scene; }

  protected:
    Scene *GetScene() const { return mScene; }

  private:
    void SetPhysicsTransform();
    void RemoveChild(Entity *aChild);
    void UpdateChildrenTransform();
    void UpdateWorldTransform();


  protected:
    glm::vec3 mCharacterPositionOffset{};
  private:
    
    std::string mName{};

    Entity *mParent = nullptr;

    Mesh *mMesh = nullptr;

    Transform mLocalTransform{};

    glm::mat4 mParentTransform = glm::mat4(1.0f);

    glm::mat4 mWorldTransform = glm::mat4(1.0f);

    std::vector<Entity *> mChildren;

    std::unique_ptr<RigidBody> mRigidBody;

    vector<std::string> mTags;

    PhysicsType mPhysicsType = PhysicsType::STATIC;


    bool mHasMesh = false;
    bool mHasCharacter = false;

    Animator *mAnimator = nullptr;
    size_t mFrameNumber = 0;
    glm::mat4 mAnimationTransform = glm::mat4(1.0f);

    bool mHasRigidBody = false;

    uint32_t mEntityID = kEntityCount++;

    bool mIsSensor = false;

    bool mIsSolid = true;

    bool mIsVisible = true;

    float mTotalTime = 0.0f;

    Scene *mScene = nullptr;
};
#endif // SCENE_ENTITY_HPP

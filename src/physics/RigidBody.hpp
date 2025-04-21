#ifndef PHYSICS_RIGIDBODY_HPP
#define PHYSICS_RIGIDBODY_HPP

#include "PhysicsManager.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

/// The rigidbody class to use in the physics system.
class RigidBody {
  public:

    /// rigidbody constructor. Recommended to set the entity to have physics in the scene file and let the scene loading handle this.
    RigidBody(JPH::BodyCreationSettings joltCreationSettings)
        : mJoltCreationSettings(joltCreationSettings) {}

    ~RigidBody();

    RigidBody(const RigidBody&) = default;
    RigidBody(RigidBody&&) = default;
    RigidBody& operator=(const RigidBody&) = default;
    RigidBody& operator=(RigidBody&&) = default;

    /// Add rigid body to physics system
    void Init(PhysicsManager &physicsManager, bool activate);

    /// Get the position of the rigid body
    glm::vec4 GetPosition() const;
    /// Set the position of the rigid body
    void SetPosition(glm::vec3 glm_position);

    // Get rotation of the rigid body using JPH::Quat instead of glm
    JPH::Quat GetRotationJolt() const;
    // Set rotation of the rigid body using JPH::Quat instead of glm
    void SetRotationJolt(const JPH::QuatArg rotation);

    /// set the rotation of the rigid body
    void SetRotation(glm::quat glm_position);
    /// Get the velocity of the rigid body
    glm::vec4 GetVelocity() const;
    /// Get the world transform of the rigid body
    glm::mat4 GetWorldTransform() const;

    /// Set the linear velocity of the rigid body
    void SetLinearVelocity(glm::vec3 glm_velocity) {
        mNewVelocity = Vec3(glm_velocity.x, glm_velocity.y, glm_velocity.z);
        updateVelocity = true;
    }
    /// Set the angular velocity of the rigid body
    void SetAngularVelocity(glm::vec3 glm_velocity)
    {
        mNewAngularVelocity = Vec3(glm_velocity.x, glm_velocity.y, glm_velocity.z);
        updateAngularVelocity = true;
    }

    void AddLinearImpulse(glm::vec3 glm_impulse) {
        mImpulse = Vec3(glm_impulse.x, glm_impulse.y, glm_impulse.z);
        addImpulse = true;
    }


    // internal
    void PrePhysicsUpdate(double deltaTime);

  public:

    JPH::BodyCreationSettings mJoltCreationSettings{};
    JPH::BodyID mBodyId{};

private:
    bool updatePosition = false;
    Vec3 mNewPosition{};
    bool updateRotation = false;
    Quat mNewRotation{};
    bool updateVelocity = false;
    Vec3 mNewVelocity{};
    bool updateAngularVelocity = false;
    Vec3 mNewAngularVelocity{};
    bool addImpulse = false;
    Vec3 mImpulse{};
};
#endif // PHYSICS_RIGIDBODY_HPP

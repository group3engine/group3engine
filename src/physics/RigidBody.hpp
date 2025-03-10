#ifndef PHYSICS_RIGIDBODY_HPP
#define PHYSICS_RIGIDBODY_HPP

#include "PhysicsManager.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

class RigidBody {
  public:
    // Enumerations for default test objects
    enum Shape { Ball, Floor };

    RigidBody(JPH::BodyCreationSettings joltCreationSettings)
        : mJoltCreationSettings(joltCreationSettings) {}

    void Init(PhysicsManager &physicsManager);

    glm::vec4 GetPosition() const;
    void SetPosition(glm::vec3 glm_position) const;
    void SetRotation(glm::quat glm_position) const;
    glm::vec4 GetVelocity() const;
    glm::mat4 GetWorldTransform() const;

    void SetLinearVelocity(glm::vec3 glm_velocity) const {
        Vec3 velocity(glm_velocity.x, glm_velocity.y, glm_velocity.z);
        PhysicsManager::get().mPhysicsSystem.GetBodyInterface().SetLinearVelocity(mBodyId, velocity);
    }

  public:
    Shape mShape{};

    JPH::BodyCreationSettings mJoltCreationSettings{};
    JPH::BodyID mBodyId{};
};
#endif // PHYSICS_RIGIDBODY_HPP

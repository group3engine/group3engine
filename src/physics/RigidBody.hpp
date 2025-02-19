#ifndef PHYSICS_RIGIDBODY_HPP
#define PHYSICS_RIGIDBODY_HPP

#include "PhysicsManager.hpp"

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

class RigidBody {
  public:
    // Enumerations for default test objects
    enum Shape { Ball, Floor };

    RigidBody(Shape shape);
    RigidBody(JPH::BodyCreationSettings joltCreationSettings)
        : mJoltCreationSettings(joltCreationSettings) {}

    void Init(PhysicsManager &physicsManager);

    glm::vec4 GetPosition() const;
    glm::vec4 GetVelocity() const;
    glm::mat4 GetWorldTransform() const;

  public:
    Shape mShape{};

    JPH::BodyCreationSettings mJoltCreationSettings{};
    JPH::BodyID mBodyId{};
};
#endif // PHYSICS_RIGIDBODY_HPP

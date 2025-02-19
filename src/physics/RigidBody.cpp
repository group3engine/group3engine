#include "RigidBody.hpp"

#include <spdlog/spdlog.h>

#include "PhysicsManager.hpp"

RigidBody::RigidBody(Shape shape) : mShape(shape) {
    if (mShape == Shape::Floor) {
        // Next we can create a rigid body to serve as the floor, we make a
        // large box Create the settings for the collision volume (the shape).
        // Note that for simple shapes (like boxes) you can also directly
        // construct a BoxShape.
        BoxShapeSettings floorShapeSettings(Vec3(100.0f, 1.0f, 100.0f));
        floorShapeSettings.SetEmbedded(); // A ref counted object on the stack (base class
                                          // RefTarget) should be marked as such to prevent it
                                          // from being freed when its reference count goes to
                                          // 0.

        // Create the shape
        ShapeSettings::ShapeResult floorShapeResult = floorShapeSettings.Create();
        ShapeRefC floorShape =
            floorShapeResult.Get(); // We don't expect an error here, but you can check
                                    // floorShapeResult for HasError() / GetError()

        // Create the settings for the body itself. Note that here you can also
        // set other properties like the restitution / friction.
        mJoltCreationSettings = {floorShape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(),
                                 EMotionType::Static, Layers::NON_MOVING};
        mJoltCreationSettings.mRestitution = 0.5f;
    } else if (mShape == Shape::Ball) {
        // Now create a dynamic body to bounce on the floor
        mJoltCreationSettings = {new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r),
                                 Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING};
    }
}

// Add rigid body to physics system
void RigidBody::Init(PhysicsManager &physicsManager) {
    if (mShape == Floor) {
        mBodyId = physicsManager.mPhysicsSystem.GetBodyInterface().CreateAndAddBody(
            mJoltCreationSettings, EActivation::DontActivate);
    } else if (mShape == Ball) {
        mBodyId = physicsManager.mPhysicsSystem.GetBodyInterface().CreateAndAddBody(
            mJoltCreationSettings, EActivation::Activate);
    }

    // Check that the physics system has not run out of bodies
    if (mBodyId.IsInvalid()) {
        SPDLOG_ERROR("Body ID invalid, the physics system has run out of bodies.");
        // Don't silently fail for now
        std::exit(EXIT_FAILURE);
    }

    physicsManager.mBodyIds.push_back(mBodyId);
}

glm::vec4 RigidBody::GetPosition() const {
    RVec3 position =
        PhysicsManager::get().mPhysicsSystem.GetBodyInterface().GetCenterOfMassPosition(mBodyId);

    // We are returning a point so the w componnet is 1.0
    glm::vec4 returnPosition = glm::vec4(position.GetX(), position.GetY(), position.GetZ(), 1.0f);

    return returnPosition;
}

glm::vec4 RigidBody::GetVelocity() const {
    Vec3 velocity =
        PhysicsManager::get().mPhysicsSystem.GetBodyInterface().GetLinearVelocity(mBodyId);

    // we are returning a vector so the w componnet is 0.0
    glm::vec4 return_velocity = glm::vec4(velocity.GetX(), velocity.GetY(), velocity.GetZ(), 0.f);

    return return_velocity;
}

glm::mat4 RigidBody::GetWorldTransform() const {
    RMat44 worldTransform =
        PhysicsManager::get().mPhysicsSystem.GetBodyInterface().GetWorldTransform(mBodyId);

    glm::mat4 returnWorldTransform;
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            returnWorldTransform[i][j] = worldTransform(i, j);
        }
    }

    return returnWorldTransform;
}

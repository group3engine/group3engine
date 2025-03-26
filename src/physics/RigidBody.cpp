#include "RigidBody.hpp"

#include <spdlog/spdlog.h>

#include "PhysicsManager.hpp"

RigidBody::~RigidBody() {
    // Remove body-entity mapping
    PhysicsManager::get().UnregisterBody(mBodyId);

    // Remove and destroy body
    PhysicsManager::get().RemoveAndDestroyBody(mBodyId);
}

// Add rigid body to physics system
void RigidBody::Init(PhysicsManager &physicsManager, bool activate) {
    if(activate)
    {
        mBodyId = physicsManager.mPhysicsSystem.GetBodyInterface().CreateAndAddBody(
        mJoltCreationSettings, EActivation::Activate);
    }
    else 
    {
        mBodyId = physicsManager.mPhysicsSystem.GetBodyInterface().CreateAndAddBody(
        mJoltCreationSettings, EActivation::DontActivate);
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

void RigidBody::SetPosition(glm::vec3 glm_position) const {
    RVec3 position(glm_position.x, glm_position.y, glm_position.z);

    PhysicsManager::get().mPhysicsSystem.GetBodyInterface().SetPosition(mBodyId, position, EActivation::DontActivate);
}

void RigidBody::SetRotation(glm::quat glm_rotation) const {
    
    Quat rotation(glm_rotation.x, glm_rotation.y, glm_rotation.z, glm_rotation.w);
    PhysicsManager::get().mPhysicsSystem.GetBodyInterface().SetRotation(mBodyId, rotation, EActivation::DontActivate);
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

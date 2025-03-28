#include "Bouncepad.hpp"
#include "CharacterEntity.hpp"
#include "Entity.hpp"
#include "Jolt/Math/MathTypes.h"
#include "PhysicsManager.hpp"
#include "glm/fwd.hpp"

void Bouncepad::OnCollisionStart(Entity *aOther)
{
    if(aOther->GetPhysicsType() == PhysicsType::KINEMATIC || aOther->GetPhysicsType() == PhysicsType::DYNAMIC)
    {
        SPDLOG_INFO("bouncepad triggered!");
        colliding = true;
        colliding_entity = aOther;
    }
    else if(aOther->CompareTag("character")){
        SPDLOG_INFO("bouncepad triggered for character!");
    }
}

void Bouncepad::Update(double deltaTime)
{
    if(colliding)
    {
        colliding_entity->GetRigidBody().AddLinearImpulse(glm::vec3(0.0, 10.0, 0.0));
        colliding = false;
    }
}

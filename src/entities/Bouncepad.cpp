#include "Bouncepad.hpp"
#include "CharacterEntity.hpp"
#include "Entity.hpp"
#include "Jolt/Math/MathTypes.h"
#include "PhysicsManager.hpp"
#include "glm/fwd.hpp"

void Bouncepad::OnCollisionStart(Entity *aOther)
{
    SPDLOG_INFO("bouncepad triggered!");
    colliding = true;
    colliding_entity = aOther;

    
}

void Bouncepad::Update(double deltaTime)
{
    if(colliding)
    {
        if(colliding_entity->GetPhysicsType() == PhysicsType::KINEMATIC || colliding_entity->GetPhysicsType() == PhysicsType::DYNAMIC)
        {
            colliding_entity->GetRigidBody().AddLinearImpulse(glm::vec3(0.0, 10.0, 0.0));
        }
        else if(colliding_entity->CompareType("character"))
        {
            static_cast<CharacterEntity*>(colliding_entity)->SetLinearVelocity(glm::vec3(0.0, 10.0, 0.0));
        }
        colliding = false;
    }
}

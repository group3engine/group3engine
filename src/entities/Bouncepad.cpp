#include "Bouncepad.hpp"
#include "CharacterEntity.hpp"
#include "Entity.hpp"
#include "Jolt/Math/MathTypes.h"
#include "PhysicsManager.hpp"
#include "glm/fwd.hpp"

void Bouncepad::OnCollisionStart(Entity *aOther)
{
    SPDLOG_INFO("bouncepad triggered!");
    if(aOther->GetPhysicsType() == PhysicsType::KINEMATIC || aOther->GetPhysicsType() == PhysicsType::DYNAMIC)
    {
        aOther->GetRigidBody().AddLinearImpulse(glm::vec3(0.0, 10.0, 0.0));
    }
    else if(aOther->CompareType("character"))
    {
        static_cast<CharacterEntity*>(aOther)->SetLinearVelocity(glm::vec3(0.0, 10.0, 0.0));
    }

    
}


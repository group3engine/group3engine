#include "SwingAxeHinge.hpp"

void SwingAxeHinge::InitPhysics() {
    GetRigidBody().Init(PhysicsManager::get(), false);

    PhysicsManager::get().RegisterEntity(this, GetRigidBody().mBodyId);
}

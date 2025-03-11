//
// Created by thomas on 10/03/25.
//

#include "MovingEntity.hpp"
void MovingEntity::Update(double deltaTime) {
    Entity::Update(deltaTime);
    timeElapsed += deltaTime;
    if(timeElapsed > timeToMove)
    {
        timeElapsed = 0.f;
        velocity = -velocity;
    }
    // set the velocity in the rigid body
    mRigidBody->SetLinearVelocity(velocity);

}
void MovingEntity::Awake() {
    start_position = GetTransform().translation;
}

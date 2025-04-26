//
// Created by thomas on 05/04/25.
//

#include "Arrow.hpp"
#include "AudioManager.hpp"

void Arrow::Awake()
{
    start_position = GetLocalTransform().translation;
}

void Arrow::Update(double deltaTime)
{
    timeElapsed += deltaTime;
    if(timeElapsed > timeToMove || timeElapsed < 0.f)
    {
        velocity = glm::vec3(0.f, 0.f, 0.f);
        GetRigidBody().SetPosition(start_position);
        SetAsInvisible();
        // set the velocity in the rigid body
        GetRigidBody().SetLinearVelocity(velocity);
    }
    else
    {
        if(velocity != startVelocity)
        {
            velocity = startVelocity;
            GetRigidBody().SetLinearVelocity(velocity);
            SetAsVisible();
            // play the arrow sound
            glm::vec3 pos = start_position;
            AudioManager::get().Play3D("arrow", pos.x, pos.y, pos.z);
        }
    }
    if(timeElapsed > timeToMove + timeToBeInvisible)
    {
        SetAsVisible();
        velocity = startVelocity;
        timeElapsed = -randomOffset;
        // set the velocity in the rigid body
        GetRigidBody().SetLinearVelocity(velocity);
    }
    glm::vec3 actualVelocity = GetRigidBody().GetVelocity();
    // rotate the arrow to face the direction of the velocity
    if(actualVelocity != glm::vec3(0.f, 0.f, 0.f))
    {
        glm::vec3 direction = glm::normalize(actualVelocity);
        glm::quat rotation = glm::rotation(glm::vec3(0.f, 0.f, -1.f), direction);
        rotation = glm::normalize(rotation);
        GetRigidBody().SetRotation(rotation);
    }

}

Arrow::Arrow()
{
    // generate a random offset
    randomOffset = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * offsetVariation;
    // subtract the offset from the time elapsed
    timeElapsed = -randomOffset;
    // subtract the offset from the invisible time
    timeToBeInvisible -= randomOffset;

}

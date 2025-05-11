//
// Created by thomas on 12/04/25.
//

#include "SinkingChild.hpp"

void SinkingChild::Update(double deltaTime)
{
    // if the player is more than 5m away, player exited = true
    if(player != nullptr)
    {
        SPDLOG_INFO("Character distance {}", glm::length(mInitialPosition - player->GetCharacterPositionOffset()));
        if(glm::length(mInitialPosition - player->GetCharacterPositionOffset()) > 7.f)
        {
            SPDLOG_INFO("Exited");
            mPlayerExited = true;
        }
    }
    // maintain the y offset from the parent
    float parentY = GetParent()->GetRigidBody().GetPosition().y;
    float newY = parentY - mInitialYOffset;
    Transform newTransform = GetLocalTransform();
    newTransform.translation.y = newY;
    SetTransform(newTransform);

    // update the state of the parent
    if(mPlayerExited)
    {
        static_cast<Sinking*>(GetParent())->SetStoodOn(false);
        mPlayerExited = false;
    }
    if(mPlayerEntered)
    {
        static_cast<Sinking*>(GetParent())->SetStoodOn(true);
        mPlayerEntered = false;
    }
}

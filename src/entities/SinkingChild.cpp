//
// Created by thomas on 12/04/25.
//

#include "SinkingChild.hpp"

void SinkingChild::Update(double deltaTime)
{
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
    }
    if(mPlayerEntered)
    {
        static_cast<Sinking*>(GetParent())->SetStoodOn(true);
    }
}

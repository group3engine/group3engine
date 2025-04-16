//
// Created by thomas on 12/04/25.
//

#include "Sinking.hpp"

void Sinking::Update(double deltaTime)
{
    if(mIsStoodOn && GetRigidBody().GetPosition().y > mMinHeight)
    {
        GetRigidBody().SetLinearVelocity({0, -mSinkingSpeed, 0});
    }
    else if(mIsStoodOn && GetRigidBody().GetPosition().y < mMinHeight)
    {
        GetRigidBody().SetLinearVelocity({0, 0, 0});
        GetRigidBody().SetPosition({GetRigidBody().GetPosition().x, mMinHeight, GetRigidBody().GetPosition().z});
    }
    if(!mIsStoodOn && GetRigidBody().GetPosition().y < mInitialHeight)
    {
        GetRigidBody().SetLinearVelocity({0, mSinkingSpeed, 0});
    }
    if(!mIsStoodOn && GetRigidBody().GetPosition().y > mInitialHeight)
    {
        GetRigidBody().SetLinearVelocity({0, 0, 0});
        GetRigidBody().SetPosition({GetRigidBody().GetPosition().x, mInitialHeight, GetRigidBody().GetPosition().z});
    }
}

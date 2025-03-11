//
// Created by thomas on 10/03/25.
//

#include "RotateOnX.hpp"
void RotateOnX::Update(double deltaTime) {
    if (!mHasFirstFrameHappened) {
        // set the angular velocity of the platform (only around y axis)
        mRigidBody->SetAngularVelocity(glm::vec3(mAngularVelocity, 0, 0));
        mHasFirstFrameHappened = true;
    }
}
//
// Created by thomas on 10/03/25.
//

#include "RotateOnX.hpp"
void RotateOnX::Awake(){
    // set the angular velocity of the platform (only around x axis)
    GetRigidBody().SetAngularVelocity(glm::vec3(mAngularVelocity, 0, 0));
}
RotateOnX::RotateOnX(float aAngularVelocity) : mAngularVelocity(aAngularVelocity) { }

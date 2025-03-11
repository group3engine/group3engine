//
// Created by thomas on 10/03/25.
//

#include "RotatingPlatform.hpp"

void RotatingPlatform::Awake() {
    // set the angular velocity of the platform (only around y axis)
    GetRigidBody().SetAngularVelocity(glm::vec3(0, mAngularVelocity, 0));
}
RotatingPlatform::RotatingPlatform(float aAngularVelocity) : mAngularVelocity(aAngularVelocity) {

}

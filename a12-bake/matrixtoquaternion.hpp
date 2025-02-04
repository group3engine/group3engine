//
// Created by thomas on 23/12/24.
//

#ifndef VULKANTIME_MATRIXTOQUATERNION_HPP
#define VULKANTIME_MATRIXTOQUATERNION_HPP

#include "glm/vec4.hpp"
#include "glm/mat3x3.hpp"
// grrr didn't need this its part of glm but I'm leaving it here anyway

glm::vec4 convertMatrixToQuaternion(glm::mat3 rotationMatrix)
{
    // w = 1/2 sqrt(1 + a11 + a22 + a33)
    // x = (a32 - a23) / 2w
    // y = (a13 - a31) / 2w
    // z = (a21 - a12) / 2w
    double w = 0.5 * sqrt(1 + rotationMatrix[0][0] + rotationMatrix[1][1] + rotationMatrix[2][2]);
    double x = (rotationMatrix[2][1] - rotationMatrix[1][2]) / (2 * w);
    double y = (rotationMatrix[0][2] - rotationMatrix[2][0]) / (2 * w);
    double z = (rotationMatrix[1][0] - rotationMatrix[0][1]) / (2 * w);
    glm::vec4 quaternion = glm::vec4(x, y, z, w);
    assert(glm::length(quaternion) == 1);
    return quaternion;

}

#endif //VULKANTIME_MATRIXTOQUATERNION_HPP

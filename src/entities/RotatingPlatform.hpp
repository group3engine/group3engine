//
// Created by thomas on 10/03/25.
//

#ifndef GROUP3ENGINE_ROTATINGPLATFORM_HPP
#define GROUP3ENGINE_ROTATINGPLATFORM_HPP
#include "Entity.hpp"

class RotatingPlatform : public Entity{
  public:
    void Update(double deltaTime) override;
  private:
    // the angular velocity of the platform
    float mAngularVelocity = 0.3f;
    bool mHasFirstFrameHappened = false;

};

#endif // GROUP3ENGINE_ROTATINGPLATFORM_HPP

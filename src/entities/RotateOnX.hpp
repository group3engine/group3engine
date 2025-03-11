//
// Created by thomas on 10/03/25.
//

#ifndef GROUP3ENGINE_ROTATEONX_HPP
#define GROUP3ENGINE_ROTATEONX_HPP
#include "Entity.hpp"
class RotateOnX : public Entity{
  public:
    void Update(double deltaTime) override;
  private:
    // the angular velocity of the entity
    float mAngularVelocity = 10.3f;
    bool mHasFirstFrameHappened = false;
};

#endif // GROUP3ENGINE_ROTATEONX_HPP

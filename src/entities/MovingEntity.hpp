//
// Created by thomas on 10/03/25.
//

#ifndef GROUP3ENGINE_MOVINGENTITY_HPP
#define GROUP3ENGINE_MOVINGENTITY_HPP

#include "Entity.hpp"

class MovingEntity : public Entity{
  public:
    MovingEntity() = default;

    void Update(double deltaTime) override;

  private:
    glm::vec3 start_position {};
    glm::vec3 end_position {};
    float timeToMove = 10.f;
    float timeElapsed = 0.f;
    bool mHasFirstFrameHappened = false;

};

#endif // GROUP3ENGINE_MOVINGENTITY_HPP

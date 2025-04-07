//
// Created by thomas on 05/04/25.
//

#ifndef GROUP3ENGINE_ARROW_HPP
#define GROUP3ENGINE_ARROW_HPP


#include "Entity.hpp"

class Arrow : public Entity {
public:
    Arrow();

    void Awake() override;

    void Update(double deltaTime) override;

private:
    glm::vec3 start_position {};
    glm::vec3 startVelocity = glm::vec3(0.f, 0, 10.f);
    glm::vec3 velocity = startVelocity;
    float timeToMove = 1.f;
    float timeToBeInvisible = 5.f;
    float timeElapsed = 0.f;
    float randomOffset = 0.f;
    const float offsetVariation = 0.5f;

};


#endif //GROUP3ENGINE_ARROW_HPP

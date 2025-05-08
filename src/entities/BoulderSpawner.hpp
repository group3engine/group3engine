//
// Created by thomas on 17/04/25.
//

#ifndef GROUP3ENGINE_BOULDERSPAWNER_HPP
#define GROUP3ENGINE_BOULDERSPAWNER_HPP
#include "Entity.hpp"

class BoulderSpawner : public Entity {
public:
    BoulderSpawner() = default;
    ~BoulderSpawner() override = default;
    void Awake() override;
    void Update(double deltaTime) override;

    void LaunchBoulder(Entity *boulder);

private:
    std::vector<Entity *> mBoulders;
    std::vector<Entity *> mActiveBoulders;

    float mBoulderCooldown = 2.0f;
    float mTimer = 0.0f;

    glm::vec3 forwards {};
    glm::vec3 spawnPoint {};

    const float impulseAmount = 10.f;
};


#endif //GROUP3ENGINE_BOULDERSPAWNER_HPP

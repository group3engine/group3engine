//
// Created by thomas on 17/04/25.
//

#ifndef GROUP3ENGINE_BOULDERSPAWNER_HPP
#define GROUP3ENGINE_BOULDERSPAWNER_HPP
#include "Entity.hpp"
enum class BoulderState
{
    eLiving,
    eInvisible
};
struct Boulder
{
    float lifeTime;
    float timer;
    float invisibleTime;
    BoulderState state;
    Entity* boulder;
};

float RandRange(float min, float max);

class BoulderSpawner : public Entity {
public:
    BoulderSpawner() = default;
    ~BoulderSpawner() override = default;
    void Awake() override;
    void Update(double deltaTime) override;

private:
    Boulder CreateBoulder(Entity* aEntity);
    void LaunchBoulder(Boulder& aBoulder);
    // the boulders
    std::vector<Boulder> mBoulders;
    // the range of lifeTimes
    float minLifeTime = 10.f; float maxLifeTime = 25.f;
    // the range of invisible times
    float minInvisibleTime = 0.5f; float maxInvisibleTime = 20.f;

    glm::vec3 forwards {};
    glm::vec3 spawnPoint {};

    const float impulseAmount = 10.f;


};


#endif //GROUP3ENGINE_BOULDERSPAWNER_HPP

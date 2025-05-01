//
// Created by thomas on 17/04/25.
//

#include "BoulderSpawner.hpp"

void BoulderSpawner::Awake()
{
    // find the boulder entities, and construct the boulders
    for (auto &entity : GetChildren())
    {
        if (entity->CompareTag("boulder"))
        {
            mBoulders.push_back(CreateBoulder(entity));
        }
    }
    // set the forwards vector to our forward
    forwards = GetWorldTransformComponents().rotation * glm::vec3(0.f, 0.f, 1.f);
    spawnPoint = GetWorldTransformComponents().translation;
}

void BoulderSpawner::Update(double deltaTime)
{
    // for each boulder, update the state
    for (auto &boulder : mBoulders)
    {
        // if the boulder is invisible, update the timer
        if (boulder.state == BoulderState::eInvisible)
        {
            boulder.timer -= deltaTime;
            if (boulder.timer <= 0.f)
            {
                boulder.state = BoulderState::eLiving;
                boulder.boulder->SetAsVisible();
                LaunchBoulder(boulder);
                boulder.timer = boulder.lifeTime;
            }
        }
        // if the boulder is living, update the timer
        else if (boulder.state == BoulderState::eLiving)
        {
            boulder.timer -= deltaTime;
            if (boulder.timer <= 0.f)
            {
                boulder.state = BoulderState::eInvisible;
                boulder.timer = boulder.invisibleTime;
                boulder.boulder->SetAsInvisible();
                boulder.boulder->GetRigidBody().SetPosition({10000000, 0, 0});
            }
        }
    }
}

Boulder BoulderSpawner::CreateBoulder(Entity *aEntity)
{
    // choose the lifetime
    float lifeTime = RandRange(minLifeTime, maxLifeTime);
    // choose the invisible time
    float invisibleTime = RandRange(minInvisibleTime, maxInvisibleTime);
    // choose the start invisible time - randomly between invisible time and 0
    float startTimer = RandRange(0.f, invisibleTime);
    // create the boulder
    Boulder boulder;
    boulder.boulder = aEntity;
    boulder.lifeTime = lifeTime;
    boulder.invisibleTime = invisibleTime;
    boulder.timer = startTimer;
    boulder.state = BoulderState::eInvisible;
    // set the boulder to invisible
    boulder.boulder->SetAsInvisible();
    return boulder;
}

void BoulderSpawner::LaunchBoulder(Boulder &aBoulder)
{
    // Set the boulders location to us
    aBoulder.boulder->GetRigidBody().SetPosition(spawnPoint);
    // add an impulse
    aBoulder.boulder->GetRigidBody().SetLinearVelocity(impulseAmount * forwards);
}

float RandRange(float min, float max)
{
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}
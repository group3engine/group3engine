//
// Created by thomas on 17/04/25.
//

#include "BoulderSpawner.hpp"

void BoulderSpawner::Awake() {
    // find the boulder entities, and construct the boulders
    for (auto *entity : GetChildren()) {
        if (entity->CompareTag("boulder")) {
            mBoulders.push_back(entity);
        }
    }
    // set the forwards vector to our forward
    forwards = GetWorldTransformComponents().rotation * glm::vec3(0.f, 0.f, 1.f);
    spawnPoint = GetWorldTransformComponents().translation;
}

void BoulderSpawner::Update(double deltaTime) {
    mTimer += deltaTime;

    // NOTE: Make sure this happens before the launching boulder code to ensure boulders aren't
    // set to invisible before their world position is updated in the PrePhysicsUpdate next frame
    // for (auto *entity : mBoulders)
    // {
    //     // Set the boulder to invisible if it is below a certain level
    //     if (entity->GetRigidBody().GetPositionJolt().GetY() < 0.0f) {
    //         entity->SetAsInvisible();
    //     }
    // }

    if (mTimer > mBoulderCooldown) {
        assert(!mBoulders.empty());
        // Launch the last boulder
        Entity *lastBoulder = mBoulders.back();
        LaunchBoulder(lastBoulder);
        // Move the last boulder to the front so other boulders are launched next
        mBoulders.insert(mBoulders.begin(), lastBoulder);
        mBoulders.pop_back();
        // Reset timer
        mTimer = 0.0f;
    }
}

void BoulderSpawner::LaunchBoulder(Entity *boulder) {
    boulder->SetAsVisible();
    // Set the boulders location to us
    boulder->GetRigidBody().SetPosition(spawnPoint);
    // Add an impulse
    boulder->GetRigidBody().SetLinearVelocity(impulseAmount * forwards);
}

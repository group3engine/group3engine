//
// Created by thomas on 17/04/25.
//

#ifndef GROUP3ENGINE_SPIKETRAP_HPP
#define GROUP3ENGINE_SPIKETRAP_HPP
#include "Entity.hpp"

enum class SpikeTrapState
{
    eWaiting,
    eHitting,
    eHit,
    eRetracting,
};

class SpikeTrap : public Entity {
public:
    SpikeTrap() = default;
    ~SpikeTrap() override = default;
    void Awake() override;
    void Update(double aDeltaTime) override;
private:
    glm::quat initialRotation {};
    glm::quat hitRotation {};
    double timer = 0.0;
    const double waitTime = 3.0;
    const double hittingTime = 0.3;
    const double hitTime = 1.0;
    const double retractTime = 2.5;
    SpikeTrapState mState = SpikeTrapState::eWaiting;

};


#endif //GROUP3ENGINE_SPIKETRAP_HPP

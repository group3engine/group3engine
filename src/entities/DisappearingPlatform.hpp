//
// Created by thomas on 26/04/25.
//

#ifndef GROUP3ENGINE_DISAPPEARINGPLATFORM_HPP
#define GROUP3ENGINE_DISAPPEARINGPLATFORM_HPP
#include "Entity.hpp"

enum class DisappearingPlatformState {
    STEPPING,
    DISAPPEARING,
    RESETTING,
    IDLE
};

class DisappearingPlatform : public Entity {
public:
    DisappearingPlatform() = default;
    ~DisappearingPlatform() override = default;

    void Awake() override;
    void Update(double deltaTime) override;

    void OnCollisionStart(Entity *other) override;

private:
    float timeToStep{};
    float timeToReset{};
    DisappearingPlatformState mState = DisappearingPlatformState::IDLE;
    glm::vec3 startScale{};
    float timeToShrink{};
    float timer = 0.f;


};


#endif //GROUP3ENGINE_DISAPPEARINGPLATFORM_HPP

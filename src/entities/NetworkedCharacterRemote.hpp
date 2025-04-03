//
// Created by thomas on 03/04/25.
//

#ifndef GROUP3ENGINE_NETWORKEDCHARACTERREMOTE_HPP
#define GROUP3ENGINE_NETWORKEDCHARACTERREMOTE_HPP
#include "Entity.hpp"
#include "SampleJoltCharacter.h"

struct State
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat rotation;
};

class NetworkedCharacterRemote : public Entity {
public:
    NetworkedCharacterRemote();
    ~NetworkedCharacterRemote() override = default;

    void ProcessInput();
    void PrePhysicsUpdate();

    Vec3 GetCharacterPosition() {
        return mSampleJoltCharacter->GetCharacterPosition();
    }

    void UpdateState(State state)
    {
        mSampleJoltCharacter->SetCharacterPosition(RVec3(state.position.x, state.position.y, state.position.z));
        Transform newTransform = GetLocalTransform();
        newTransform.rotation = glm::normalize(state.rotation);
        SetTransform(newTransform);
        mSampleJoltCharacter->SetCharacterVelocity(RVec3(state.velocity.x, state.velocity.y, state.velocity.z));
    }

    // update override
    void LateUpdate(double deltaTime) override;


    void CreateJoltCharacter();

    void Awake() override ;



    [[nodiscard]] glm::vec3 GetCharacterPositionOffset() const { return mCharacterPositionOffset; }
    void SetCharacterPositionOffset(glm::vec3 aPosition) { mCharacterPositionOffset = aPosition; }
    void SetCharacterPositionOffset(float x, float y, float z) {
        mCharacterPositionOffset = glm::vec3(x, y, z);
    }

    void AddImpulse(glm::vec3 glm_impulse) {
        Vec3 impulse(glm_impulse.x, glm_impulse.y, glm_impulse.z);
        mSampleJoltCharacter->AddImpulse(impulse);
    }


    std::unique_ptr<SampleJoltCharacter> mSampleJoltCharacter;

};


#endif //GROUP3ENGINE_NETWORKEDCHARACTERREMOTE_HPP

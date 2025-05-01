//
// Created by thomas on 12/04/25.
//

#ifndef GROUP3ENGINE_SINKINGCHILD_HPP
#define GROUP3ENGINE_SINKINGCHILD_HPP
#include "Entity.hpp"
#include "Sinking.hpp"
#include "CharacterEntity.hpp"


class SinkingChild : public Entity{
public:
    SinkingChild() = default;
    ~SinkingChild() = default;
    void Awake() override {
        mInitialYOffset = GetParent()->GetRigidBody().GetPosition().y + GetLocalTransform().translation.y;
        mInitialPosition = GetWorldTransformComponents().translation;
    }
    void Update(double deltaTime) override;


    void OnCollisionStart(Entity* other) {
        if(other->IsCharacter()) {
            mPlayerEntered = true;
            player = static_cast<CharacterEntity*>(other);
        }
    }

    void OnCollisionStay(Entity* other) {
        if(other->IsCharacter()) {
            mPlayerEntered = true;
            player = static_cast<CharacterEntity*>(other);
        }
    }

private:
    float mInitialYOffset = 0.f;
    bool mPlayerEntered = false;
    bool mPlayerExited = false;
    CharacterEntity* player = nullptr;
    glm::vec3 mInitialPosition;

};


#endif //GROUP3ENGINE_SINKINGCHILD_HPP

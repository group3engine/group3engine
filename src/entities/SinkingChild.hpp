//
// Created by thomas on 12/04/25.
//

#ifndef GROUP3ENGINE_SINKINGCHILD_HPP
#define GROUP3ENGINE_SINKINGCHILD_HPP
#include "Entity.hpp"
#include "Sinking.hpp"

class SinkingChild : public Entity{
public:
    SinkingChild() = default;
    ~SinkingChild() = default;
    void Awake() override {
        mInitialYOffset = GetParent()->GetRigidBody().GetPosition().y + GetLocalTransform().translation.y;
    }
    void Update(double deltaTime) override;

    void OnCollisionEnd(Entity* other) {
         if(other->IsCharacter()) {
             mPlayerExited = true;
         }
    }
    void OnCollisionStart(Entity* other) {
        if(other->IsCharacter()) {
            mPlayerEntered = true;
        }
    }

    void OnCollisionStay(Entity* other) {
        if(other->IsCharacter()) {
            mPlayerEntered = true;
        }
    }

private:
    float mInitialYOffset = 0.f;
    bool mPlayerEntered = false;
    bool mPlayerExited = false;

};


#endif //GROUP3ENGINE_SINKINGCHILD_HPP

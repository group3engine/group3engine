//
// Created by thomas on 12/04/25.
//

#ifndef GROUP3ENGINE_SINKING_HPP
#define GROUP3ENGINE_SINKING_HPP
#include "Entity.hpp"

class Sinking : public Entity {
public:
    Sinking() = default;
    ~Sinking() override = default;
    void Awake() override{mInitialHeight = GetRigidBody().GetPosition().y; mMinHeight = mInitialHeight - 5.f; mInitialPosition = GetRigidBody().GetPosition();}
    void Update(double deltaTime) override;

    void SetStoodOn(bool stoodOn) {
        mIsStoodOn = stoodOn;
    }

private:
    bool mIsStoodOn = false;

    float mInitialHeight = 0.f;
    float mMinHeight = 0.f;
    float mSinkingSpeed = 0.5f;
    glm::vec3 mInitialPosition {};

};


#endif //GROUP3ENGINE_SINKING_HPP

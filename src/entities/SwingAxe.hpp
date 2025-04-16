#ifndef GROUP3ENGINE_SWINGAXE_HPP
#define GROUP3ENGINE_SWINGAXE_HPP

#include "Entity.hpp"

#include <Jolt/Physics/Constraints/HingeConstraint.h>

class SwingAxe : public Entity {
  public:
    void InitPhysics() override;

    // void Awake() override;

    void Update(double deltaTime) override;

    double swingTime = 0.0f;

    JPH::HingeConstraint *mConstraint;
};
#endif // GROUP3ENGINE_SWINGAXE_HPP

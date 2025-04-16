#ifndef GROUP3ENGINE_SWINGAXEHINGE_HPP
#define GROUP3ENGINE_SWINGAXEHINGE_HPP

#include "Entity.hpp"

class SwingAxeHinge : public Entity {
  public:
    SwingAxeHinge() { mType = "swing_axe_hinge"; }

    void InitPhysics() override;

    // void Awake() override;

    // void Update(double deltaTime) override;
};
#endif // GROUP3ENGINE_SWINGAXEHINGE_HPP


#ifndef GROUP3ENGINE_BOUNEPAD_HPP
#define GROUP3ENGINE_BOUNEPAD_HPP

#include "Entity.hpp"

class Bouncepad : public Entity{
    public:
        Bouncepad() = default;

        void OnCollisionStart(Entity *aOther) override;
        void Update(double deltaTime) override;
    
    private:
        bool colliding = false;
        Entity *colliding_entity;
};

#endif // GROUP3ENGINE_BOUNEPAD_HPP

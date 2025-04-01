
#ifndef GROUP3ENGINE_BOUNEPAD_HPP
#define GROUP3ENGINE_BOUNEPAD_HPP

#include "Entity.hpp"

class Bouncepad : public Entity{
    public:
        Bouncepad() = default;

        void OnCollisionStart(Entity *aOther) override;

};

#endif // GROUP3ENGINE_BOUNEPAD_HPP

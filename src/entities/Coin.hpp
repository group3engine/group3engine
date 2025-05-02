#ifndef GROUP3ENGINE_COIN_HPP
#define GROUP3ENGINE_COIN_HPP

#include "Entity.hpp"

class Coin : public Entity {

    public:
        void Awake() override;

        void OnCollisionStart(Entity *other) override;

        void SetCoinCounter(int* aCoinCount);
    
    private:
        bool mCollected = false;

};
#endif // GROUP3ENGINE_IDOL_HPP
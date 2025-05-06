#ifndef GROUP3ENGINE_COIN_HPP
#define GROUP3ENGINE_COIN_HPP

#include "Entity.hpp"

enum class CoinState {
    COLLECTED,
    DISAPPEARING,
    UNCOLLECTED
};

class Coin : public Entity {

    public:
        void Awake() override;

        void OnCollisionStart(Entity *other) override;

        void Update(double deltaTime) override;

        void SetCoinCounter(int* aCoinCount);

        void Collect();
    
    private:
        CoinState mCollected = CoinState::UNCOLLECTED;
        float timeToShrink = 0.3f;
        glm::vec3 startScale{};
        float timer = 0.f;
        bool mCollectedOnStart = false;

};
#endif // GROUP3ENGINE_IDOL_HPP
//
// Created by thomas on 07/04/25.
//

#ifndef GROUP3ENGINE_TILEMANAGER_HPP
#define GROUP3ENGINE_TILEMANAGER_HPP
#include "Entity.hpp"
#include "TileBox.hpp"
class TileManager : public Entity{
public:
    TileManager() = default;
    void Awake() override;
    void Update(double aDeltaTime) override;

private:
    std::vector<TileBox*> mTileBoxes {};
    Entity* mMonkey = nullptr;

};


#endif //GROUP3ENGINE_TILEMANAGER_HPP

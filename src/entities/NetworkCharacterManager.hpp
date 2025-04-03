//
// Created by thomas on 02/04/25.
//

#ifndef GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP
#define GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP
#include "Entity.hpp"
#include "Networking.hpp"
#include "NetworkedCharacterRemote.hpp"

class NetworkCharacterManager : public Entity {
public:
    NetworkCharacterManager(){mType = "NetworkCharacterManager";};
    ~NetworkCharacterManager() override = default;

    void Update(double deltaTime) override;
    void SendMessage(const std::string &message) {
        mNetworking.SendMessage(message);
    }


private:
    Networking mNetworking;
    // map of player id to child index
    std::unordered_map<uint32_t, size_t> mPlayerIdToChildIndex;
    size_t numConnections = 0;


};


#endif //GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP

//
// Created by thomas on 02/04/25.
//

#ifndef GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP
#define GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP
#include "Entity.hpp"
#include "Networking.hpp"

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


};


#endif //GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP

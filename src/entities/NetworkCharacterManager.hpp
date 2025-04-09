//
// Created by thomas on 02/04/25.
//

#ifndef GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP
#define GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP
#include "Entity.hpp"
#include "Networking.hpp"
#include "NetworkedCharacterRemote.hpp"
#include "ImGuiRenderer.hpp"

class NetworkCharacterManager : public Entity {
public:
    NetworkCharacterManager(){mType = "NetworkCharacterManager";};
    ~NetworkCharacterManager() override = default;

    void Update(double deltaTime) override;
    void SendMessage(const std::string &message) {
        mNetworking.SendMessage(message);
    }

    void SendChatMessage(std::string playerName, std::string message);

    void UpdateUi(double deltaTime) override {
        std::vector<Message> sampleMessages = {
                {"Alice", "Hello, world!", "10:00"},
                {"Bob", "How are you?", "10:01"},
                {"Charlie", "Good morning!", "10:02"},
                {"David", "Nice to meet you!", "10:03"},
                {"Eve", "Welcome to the game.", "10:04"},
                {"Frank", "Let's start the quest.", "10:05"},
                {"Grace", "Watch out for enemies.", "10:06"},
                {"Heidi", "I found a secret path.", "10:07"},
                {"Ivan", "Collect all the treasures.", "10:08"},
                {"Judy", "Good luck, everyone!", "10:09"}
        };
        ImGuiRenderer::ChatWindow(sampleMessages, [this](const std::string &playerName, const std::string &message) {
            SendChatMessage(playerName, message);
        });
    }


private:
    Networking mNetworking;
    // map of player id to child index
    std::unordered_map<uint32_t, size_t> mPlayerIdToChildIndex;
    size_t numConnections = 0;


};


#endif //GROUP3ENGINE_NETWORKCHARACTERMANAGER_HPP

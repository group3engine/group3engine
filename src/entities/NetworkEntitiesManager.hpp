//
// Created by thomas on 02/04/25.
//

#ifndef GROUP3ENGINE_NETWORKENTITIESMANAGER_HPP
#define GROUP3ENGINE_NETWORKENTITIESMANAGER_HPP
#include "Entity.hpp"
#include "Networking.hpp"
#include "NetworkedCharacterRemote.hpp"
#include "ImGuiRenderer.hpp"
namespace JSONPARSE
{
    std::tuple<std::string, std::string> GetPairFromString(const std::string &aString);
}

class NetworkEntitiesManager : public Entity {
public:
    NetworkEntitiesManager();
    ~NetworkEntitiesManager() override;

    void Update(double deltaTime) override;
    void SendMessage(const std::array<char, BUFFER_SIZE> &message, size_t size) {
        mNetworking.SendMessage(message, size);
    }

    void SendChatMessage(std::string playerName, std::string message);

    void ReceiveMessages();

    void UpdateUi(double deltaTime) override {
        std::lock_guard<std::mutex> lock(messages_mutex);
        ImGuiRenderer::ChatWindow(mChatMessages, [this](const std::string &playerName, const std::string &message) {
            SendChatMessage(playerName, message);
        });
    }


private:
    Networking mNetworking;
    // map of player id to child index
    std::unordered_map<std::string, size_t> mPlayerIdToChildIndex;
    size_t numConnections = 0;

    std::thread chatGetThread;
    std::vector<Message> mChatMessages{};
    std::mutex messages_mutex;
    bool chatting = true;



};


#endif //GROUP3ENGINE_NETWORKENTITIESMANAGER_HPP

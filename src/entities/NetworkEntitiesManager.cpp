//
// Created by thomas on 02/04/25.
//

#include "NetworkEntitiesManager.hpp"
#include "Input.hpp"
#include "Scene.hpp"

#include "Debugging.hpp"
#include "NetworkSignals.hpp"
#include "SignalSystem.hpp"

void NetworkEntitiesManager::Update(double deltaTime)
{
    // get the messages
    auto messages = mNetworking.GetMessages();
    // create a map of states
    std::unordered_map<std::string, State> states;
    std::vector<std::string> playerIDs;
    // for each message
    for(auto &message : messages)
    {
        std::string playerID = "";
        State state{};
        std::string messageString{};
        try {

            // split the message data into the json and the player id
            messageString = std::string(message.data());
            playerID = std::string(message.data() + messageString.size() + 2);
            nlohmann::json jsonData = nlohmann::json::parse(messageString);
            // construct the state
            state.position = {
                    jsonData["transform"]["position"][0],
                    jsonData["transform"]["position"][1],
                    jsonData["transform"]["position"][2]
            };
            state.rotation = {
                    jsonData["transform"]["rotation"][0],
                    jsonData["transform"]["rotation"][1],
                    jsonData["transform"]["rotation"][2],
                    jsonData["transform"]["rotation"][3]
            };
            state.velocity = {
                    jsonData["velocity"][0],
                    jsonData["velocity"][1],
                    jsonData["velocity"][2]
            };
            state.isCrouching = jsonData["isCrouching"];
            state.isEmoting = jsonData["isEmoting"];
            state.isInClimb = jsonData["isInClimb"];
            state.deathState = static_cast<DeathState>(jsonData["deathState"]);
            state.isHanging = jsonData["isHanging"];


            // TODO: Add code to respond to this
            // nlohmann::json idols = jsonData["idols"];
            // DEBUG_ASSERT(idols.is_array());
            // for (const auto& idolData : idols) {
            //     DEBUG_ASSERT(idolData.is_object());
            //     // wasCollected will always be true for now but other data could be added
            //     NetworkIdolSignal networkIdolSignal{};
            //     networkIdolSignal.entityID = idolData["entityID"];
            //     networkIdolSignal.wasCollected = idolData["wasCollected"];
            //     GetScene()->mSignalSystem.EmitSignal(&networkIdolSignal);
            // }

            // For most entities there are multiple of them so we should have a
            // json array and check to see if the json array is empty or not.
            // Emitting one signal per element in the array
            // Even for the idol where there should be one, do the same thing for simplicity
            if (auto levers = jsonData.find("levers"); levers != jsonData.end()) {
                DEBUG_ASSERT(levers->is_array());
                for (const auto& leverData : *levers) {
                    DEBUG_ASSERT(leverData.is_object());
                    // wasPulled will always be true for now but other data could be added
                    NetworkLeverSignal networkLeverSignal{};
                    networkLeverSignal.entityID = leverData["entityID"];
                    networkLeverSignal.wasPulled = leverData["wasPulled"];
                    GetScene()->mSignalSystem.EmitSignal(&networkLeverSignal);
                }
            }
        }
        catch(const nlohmann::json::parse_error &e)
        {
            SPDLOG_ERROR("Parse error: {}", e.what());
            SPDLOG_ERROR("Message: {}", messageString);
            continue;
        }
        catch(const std::exception &e)
        {
            SPDLOG_ERROR("Exception: {}", e.what());
            continue;
        }

        // add the state to the map
        states[playerID] = state;
        // see if the player id is in the map
        if(mPlayerIdToChildIndex.find(playerID) == mPlayerIdToChildIndex.end())
        {
            if(numConnections >= GetChildren().size()-1)
            {
                continue;
            }
            // if not, add it to the map
            mPlayerIdToChildIndex[playerID] = ++numConnections;
        }
        // add the player id to the list
        playerIDs.push_back(playerID);
    }
    // apply the states to the children
    for(auto &playerID : playerIDs)
    {
        // get the child index
        size_t childIndex = mPlayerIdToChildIndex[playerID];
        // get the state
        State state = states[playerID];
        // get the child
        NetworkedCharacterRemote *child = static_cast<NetworkedCharacterRemote*>(GetChildren()[childIndex]);
        // update the state
        child->UpdateState(state);
    }
}

void NetworkEntitiesManager::LateUpdate(double deltaTime) {
    std::string mapName = Scene::get().GetActiveScene()->GetSceneFilename().string();

    std::string jsonToSend = mLocalJson.dump();

    // SPDLOG_INFO("{}", jsonToSend);

    // add the map name to the start for quick parsing
    std::array<char, BUFFER_SIZE> buffer;
    std::copy(mapName.begin(), mapName.end(), buffer.data());
    buffer[mapName.size()] = '\0';
    // add the json to the end
    std::copy(jsonToSend.begin(), jsonToSend.end(), buffer.data() + mapName.size() + 1);
    SendMessage(buffer, mapName.size() + 1 + jsonToSend.size());

    // Clear local json after sending
    mLocalJson.clear();
}

void NetworkEntitiesManager::SendChatMessage(std::string playerName, std::string message)
{
    // generate the json of the message
    // we need to include the player name, message, timestamp, and map name
    std::string mapName = Scene::get().GetActiveScene()->GetSceneFilename().string();
    std::string jsonToSend = "{ \"playerName\": \"" + playerName + "\", \"message\": \"" + message + "\", \"time\": \"" + std::to_string(time(nullptr)) + "\", \"gameID\": \"" + mapName + "\" }";
    // send the message
    std::string url = "wipeoutchat.pythonanywhere.com/SendMessage";
    // send the message in a thread
    // join the thread
    std::thread chatSendThread = std::thread(http_post, url, jsonToSend);
    chatSendThread.detach();
}

NetworkEntitiesManager::NetworkEntitiesManager()
{
    mType = "NetworkEntitiesManager";
    // start off the receive thread
    chatGetThread = std::thread(&NetworkEntitiesManager::ReceiveMessages, this);
}

NetworkEntitiesManager::~NetworkEntitiesManager()
{
    chatting = false;
    if (chatGetThread.joinable())
    {
        chatGetThread.join();
    }

}

void NetworkEntitiesManager::ReceiveMessages()
{
    while(chatting)
    {
        // construct the url
        std::string url = "wipeoutchat.pythonanywhere.com/GetMessages?gameID=" + Scene::get().GetActiveScene()->GetSceneFilename().string();
        // get the chat
        auto messages = http_get(url);
        try {
            nlohmann::json data = nlohmann::json::parse(messages);
            std::vector<Message> messages;
            messages.reserve(data.size());
            // for each element in the json array
            for (const auto &item : data) {

                // get the player name
                std::string playerName = item["playerName"];
                // get the message
                std::string message = item["message"];
                // get the timestamp
                std::string timestamp = item["time"];
                // add the message to the vector
                messages.emplace_back(playerName, message, timestamp);
            }
            // copy the messages to the chat messages
            {
                std::lock_guard<std::mutex> lock(messages_mutex);
                mChatMessages = messages;
            }
        }
        catch (const nlohmann::json::parse_error &e) {
            SPDLOG_ERROR("Parse error: {}", e.what());
            continue;
        }
        // sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

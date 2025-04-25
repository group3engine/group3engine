//
// Created by thomas on 02/04/25.
//

#include "NetworkCharacterManager.hpp"
#include "Input.hpp"
#include "Scene.hpp"
#include <json.hpp>



void NetworkCharacterManager::Update(double deltaTime)
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

void NetworkCharacterManager::SendChatMessage(std::string playerName, std::string message)
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

NetworkCharacterManager::NetworkCharacterManager()
{
    mType = "NetworkCharacterManager";
    // start off the receive thread
    chatGetThread = std::thread(&NetworkCharacterManager::ReceiveMessages, this);
}

NetworkCharacterManager::~NetworkCharacterManager()
{
    chatting = false;
    if (chatGetThread.joinable())
    {
        chatGetThread.join();
    }

}

void NetworkCharacterManager::ReceiveMessages()
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

std::tuple<std::string, std::string>  JSONPARSE::GetPairFromString(const std::string &aString)
{
    // split the string by :
    size_t colonPos = aString.find(':');
    std::string key = aString.substr(0, colonPos);
    std::string value = aString.substr(colonPos + 1);
    // take the string between the first " and the last "
    size_t firstQuote = value.find_first_of('"');
    size_t lastQuote = value.find_last_of('"');
    value = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
    // same for the key
    firstQuote = key.find_first_of('"');
    lastQuote = key.find_last_of('"');
    key = key.substr(firstQuote + 1, lastQuote - firstQuote - 1);

    return std::make_tuple(key, value);
}

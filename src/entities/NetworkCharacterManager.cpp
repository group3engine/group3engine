//
// Created by thomas on 02/04/25.
//

#include "NetworkCharacterManager.hpp"
#include "Input.hpp"
#include "Scene.hpp"



void NetworkCharacterManager::Update(double deltaTime)
{
    // get the messages
    auto messages = mNetworking.GetMessages();
    // create a map of states
    std::unordered_map<uint32_t, State> states;
    uint32_t samplePlayerID = 0;
    // for each message
    for(auto &message : messages)
    {
        // print the message
        std::string messageString(message.data());
        // get the location of the last } in the message
        size_t lastBracket = messageString.find_last_of('}');
        // get the player id from the characters after the last }
        std::string playerId = messageString.substr(lastBracket + 1);
        // convert the player id to an int by the bits
        uint32_t playerIDint;
        std::memcpy(&playerIDint, playerId.data(), sizeof(playerIDint)); // Copy first 4 bytes
        std::cout << "Player ID: " << playerIDint << std::endl;
        // get the json string - this is the part between the first { and the last }
        std::string jsonString = messageString.substr(messageString.find_first_of('{'), lastBracket - messageString.find_first_of('{') + 1);
        // parse the json string
        // get the position - the values between [ and ] after "position":
        size_t posStart = jsonString.find("[", jsonString.find("\"position\":"));
        size_t posEnd = jsonString.find("]", posStart);
        std::string posString = jsonString.substr(posStart + 1, posEnd - posStart - 1);
        // split the string by ,
        std::vector<std::string> posValues;
        std::stringstream ss(posString);
        std::string value;
        while (std::getline(ss, value, ','))
        {
            posValues.push_back(value);
        }
        // convert the values to floats
        float posX = std::stof(posValues[0]);
        float posY = std::stof(posValues[1]);
        float posZ = std::stof(posValues[2]);
        glm::vec3 position = glm::vec3(posX, posY, posZ);
        std::cout << "Position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        // get the rotation - the values between [ and ] after "rotation":
        size_t rotStart = jsonString.find("[", jsonString.find("\"rotation\":"));
        size_t rotEnd = jsonString.find("]", rotStart);
        std::string rotString = jsonString.substr(rotStart + 1, rotEnd - rotStart - 1);
        // split the string by ,
        std::vector<std::string> rotValues;
        std::stringstream ss2(rotString);
        std::string rotValue;
        while (std::getline(ss2, rotValue, ','))
        {
            rotValues.push_back(rotValue);
        }
        // convert the values to floats
        float rotX = std::stof(rotValues[0]);
        float rotY = std::stof(rotValues[1]);
        float rotZ = std::stof(rotValues[2]);
        float rotW = std::stof(rotValues[3]);
        glm::quat rotation = glm::quat(rotX, rotY, rotZ, rotW);
        std::cout << "Rotation: " << rotation.x << ", " << rotation.y << ", " << rotation.z << ", " << rotation.w << std::endl;
        // get the scale - the values between [ and ] after "scale":
        size_t scaleStart = jsonString.find("[", jsonString.find("\"scale\":"));
        size_t scaleEnd = jsonString.find("]", scaleStart);
        std::string scaleString = jsonString.substr(scaleStart + 1, scaleEnd - scaleStart - 1);
        // split the string by ,
        std::vector<std::string> scaleValues;
        std::stringstream ss3(scaleString);
        std::string scaleValue;
        while (std::getline(ss3, scaleValue, ','))
        {
            scaleValues.push_back(scaleValue);
        }
        // convert the values to floats
        float scaleX = std::stof(scaleValues[0]);
        float scaleY = std::stof(scaleValues[1]);
        float scaleZ = std::stof(scaleValues[2]);
        glm::vec3 scale = glm::vec3(scaleX, scaleY, scaleZ);
        std::cout << "Scale: " << scale.x << ", " << scale.y << ", " << scale.z << std::endl;
        // get the velocity - the values between [ and ] after "velocity":
        size_t velStart = jsonString.find("[", jsonString.find("\"velocity\":"));
        size_t velEnd = jsonString.find("]", velStart);
        std::string velString = jsonString.substr(velStart + 1, velEnd - velStart - 1);
        // split the string by ,
        std::vector<std::string> velValues;
        std::stringstream ss4(velString);
        std::string velValue;
        while (std::getline(ss4, velValue, ','))
        {
            velValues.push_back(velValue);
        }
        // convert the values to floats
        float velX = std::stof(velValues[0]);
        float velY = std::stof(velValues[1]);
        float velZ = std::stof(velValues[2]);
        glm::vec3 velocity = glm::vec3(velX, velY, velZ);
        std::cout << "Velocity: " << velocity.x << ", " << velocity.y << ", " << velocity.z << std::endl;
        // construct the state
        State state;
        state.position = position;
        state.rotation = rotation;
        state.velocity = velocity;
        // add the state to the map
        states[playerIDint] = state;
        samplePlayerID = playerIDint;
    }
    // apply the first state to the second child
    NetworkedCharacterRemote* character = static_cast<NetworkedCharacterRemote*>(GetChildren()[1]);
    if(states.find(samplePlayerID) == states.end())
    {

    }
    else
    {
    character->UpdateState(states[samplePlayerID]);
    }
}


//
// Created by thomas on 02/04/25.
//

#include "NetworkCharacterManager.hpp"
#include "Input.hpp"
#include "Scene.hpp"

void NetworkCharacterManager::Update(double deltaTime)
{

    if(IsKeyDown(KEY::eO))
    {
        // get the messages
        auto messages = mNetworking.GetMessages();
        // for each message
        for(auto &message : messages)
        {
            // print the message
            std::cout << message.data() << std::endl;
        }
    }
}

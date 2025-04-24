//
// Created by thomas on 03/04/25.
//

#include "NetworkedLocalCharacter.hpp"
#include "NetworkCharacterManager.hpp"
#include "Scene.hpp"

void NetworkedLocalCharacter::Update(double deltaTime)
{
    // construct our state - we want to include the current transform, and the current velocity
    Transform transform = GetLocalTransform();
    Vec3 position = mSampleJoltCharacter->GetCharacterPosition();
    Vec3 jvelocity = mSampleJoltCharacter->GetCharacterVelocity();
    glm::vec3 velocity = glm::vec3(jvelocity.GetX(), jvelocity.GetY(), jvelocity.GetZ());
    // get the file name
    std::string mapName = Scene::get().GetActiveScene()->GetSceneFilename().string();
    // jsonify
    std::string jsonToSend = "{ \"transform\": { \"position\": [" + to_string(position.GetX()) + "," + to_string(position.GetY()) + "," + to_string(position.GetZ()) + "], \"rotation\": [" + to_string(transform.rotation.x) + "," + to_string(transform.rotation.y) + "," + to_string(transform.rotation.z) + "," + to_string(transform.rotation.w) + "," + "], \"scale\": [" + to_string(transform.scale.x) + "," + to_string(transform.scale.y) + "," + to_string(transform.scale.z) + "] }, \"velocity\": [" + to_string(velocity.x) + "," + to_string(velocity.y) + "," + to_string(velocity.z) + "], \"mapName\": \"" + mapName + "\" + \"isCrouching\": " + std::to_string(mIsCrouching) + ", \"isEmoting\": " + std::to_string(mIsEmoting) + ", \"isInClimb\": " + std::to_string(mInClimb) + "}";
    // add the map name to the start for quick parsing
    std::array<char, BUFFER_SIZE> buffer;
    std::copy(mapName.begin(), mapName.end(), buffer.data());
    buffer[mapName.size()] = '\0';
    // add the json to the end
    std::copy(jsonToSend.begin(), jsonToSend.end(), buffer.data() + mapName.size() + 1);
    static_cast<NetworkCharacterManager*>(GetParent())->SendMessage(buffer, mapName.size() + 1 + jsonToSend.size());

    CharacterEntity::Update(deltaTime);
}

NetworkedLocalCharacter::NetworkedLocalCharacter() :
    CharacterEntity()
{
    mType = "NetworkedLocalCharacter";
}

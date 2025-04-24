//
// Created by thomas on 03/04/25.
//

#include "NetworkedLocalCharacter.hpp"
#include "NetworkCharacterManager.hpp"
#include "Scene.hpp"
#include <json.hpp>

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
    nlohmann::json jsonData;
    jsonData["transform"] = {
            {"position", {position.GetX(), position.GetY(), position.GetZ()}},
            {"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w}},
            {"scale", {transform.scale.x, transform.scale.y, transform.scale.z}}
    };
    jsonData["velocity"] = {velocity.x, velocity.y, velocity.z};
    jsonData["mapName"] = mapName;
    jsonData["isCrouching"] = mIsCrouching;
    jsonData["isEmoting"] = mIsEmoting;
    jsonData["isInClimb"] = mInClimb;
    std::string jsonToSend = jsonData.dump();
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

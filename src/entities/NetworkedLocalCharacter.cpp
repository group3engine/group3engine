//
// Created by thomas on 03/04/25.
//

#include "NetworkedLocalCharacter.hpp"
#include "NetworkEntitiesManager.hpp"
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

    nlohmann::json &jsonData = GetScene()->GetNetworkEntitiesManager()->GetLocalJson();

    jsonData["transform"] = {
            {"position", {position.GetX(), position.GetY(), position.GetZ()}},
            {"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w}},
            {"scale", {transform.scale.x, transform.scale.y, transform.scale.z}}
    };
    jsonData["velocity"] = {velocity.x, velocity.y, velocity.z};
    // TODO: Is this redundant and is this in a different location? Would that be problematic?
    jsonData["mapName"] = mapName;
    jsonData["isCrouching"] = mIsCrouching;
    jsonData["isEmoting"] = mIsEmoting;
    jsonData["isInClimb"] = mInClimb;
    jsonData["deathState"] = mDeathState;

    CharacterEntity::Update(deltaTime);
}

NetworkedLocalCharacter::NetworkedLocalCharacter() :
    CharacterEntity()
{
    mType = "NetworkedLocalCharacter";
}

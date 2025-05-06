#include "Coin.hpp"

#include "Scene.hpp"
#include "SignalSystem.hpp"
#include "Signals.hpp"
#include "AudioManager.hpp"
#include "Saving.hpp"
#include "spdlog/spdlog.h"


void Coin::Awake() {
    // set the angular velocity of the platform (only around y axis)
    GetRigidBody().SetAngularVelocity(glm::vec3(0, 1, 0));
    if(Saving::get().Get<bool>(GetName()+"collected") == true)
    {
        GetRigidBody().SetActive(false);
        SetAsInvisible();
        mCollectedOnStart = true;
    }

    // get the start scale
    startScale = GetLocalTransform().scale;
}

void Coin::OnCollisionStart(Entity *other)
{
    // check that the other entity is a player
    if (other->IsCharacter())
    {
        // if it isnt collected (could be collected or dissapearing)
        if(mCollected == CoinState::UNCOLLECTED)
        {
            // set it to collected
            mCollected = CoinState::DISAPPEARING;
            Collect();
        }
    }
}

void Coin::Update(double deltaTime)
{
    if(mCollectedOnStart)
    {
        Collect();
        mCollectedOnStart = false;
    }
    if(mCollected == CoinState::DISAPPEARING)
    {
        float scaleMultiplier = 1.f;
        timer += deltaTime;
        scaleMultiplier = std::max(1.f - (timer / timeToShrink), 0.f);
        
        // set the scale of the entity
        glm::vec3 scale = startScale * scaleMultiplier;
        Transform transform = GetLocalTransform();
        transform.scale = scale;
        SetTransform(transform);
        // if the scale isn't 1, turn off the rigid body
        if (scaleMultiplier < 0.f) 
        {
            GetRigidBody().SetActive(false);
            SetAsInvisible();
            mCollected = CoinState::COLLECTED;
        }
    }
}

void Coin::Collect()
{
    // send the signal over to the character to add a coin to them
    CoinSignal coinSignal = {};
    coinSignal.transmitter = this; // this transmits it
    GetScene()->mSignalSystem.EmitSignal(&coinSignal); // send the signal over
    SPDLOG_INFO("sent signal");
    // play coin collection sound
    glm::vec3 pos = GetLocalTransform().translation; // get the current postition 
    AudioManager::get().Play3D("arrow", pos.x, pos.y, pos.z); //arrow is placeholder


    //file saving
    Saving::get().Save(GetName()+"collected", true);
}
#include "Coin.hpp"

#include "Scene.hpp"
#include "SignalSystem.hpp"
#include "Signals.hpp"
#include "AudioManager.hpp"

void Coin::Awake() {
    // set the angular velocity of the platform (only around y axis)
    GetRigidBody().SetAngularVelocity(glm::vec3(0, 1, 0));
}

void Coin::OnCollisionStart(Entity *other)
{
    // check that the other entity is a player
    if (other->IsCharacter())
    {
        if(!mCollected)
        {
            // set it to collected
            mCollected = true;

            SetAsNotSolid(); // bye bye coin
            SetAsInvisible();

            // send the signal over to the character to add a coin to them
            CoinSignal coinSignal = {};
            coinSignal.transmitter = this; // this transmits it
            coinSignal.receiver = other; // the character recieves it
            GetScene()->mSignalSystem.EmitSignal(&coinSignal); // send the signal over

            // play coin collection sound
            glm::vec3 pos = GetLocalTransform().translation; // get the current postition 
            AudioManager::get().Play3D("arrow", pos.x, pos.y, pos.z); //arrow is placeholder
        }


    }
}
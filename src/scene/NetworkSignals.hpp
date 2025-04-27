#ifndef NETWORKSIGNALS_HPP
#define NETWORKSIGNALS_HPP

#include "SignalSystem.hpp"

class Entity;

enum class ENetworkSignalActive {
    Active,
    NotActive
};

// struct NetworkSignalBase : public SignalBase<NetworkSignalBase> {
//     // ENetworkSignalActive networkSignalActive = ENetworkSignalActive::NotActive;
// };

// Define network state to read in and send as well as the signal in one
struct NetworkIdolSignal : public SignalBase<NetworkIdolSignal> {
    // Want to have multiple levers / entities in the scene
    // Trust that the loading of entities and entity IDs are consistent across clients
    uint32_t entityID = 0;

    bool wasCollected = false;
};

struct NetworkLeverSignal : public SignalBase<NetworkLeverSignal> {
    // Want to have multiple levers / entities in the scene
    // Trust that the loading of entities and entity IDs are consistent across clients
    uint32_t entityID = 0;

    bool wasPulled = false;
};
#endif // NETWORKSIGNALS_HPP

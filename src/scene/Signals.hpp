#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include "SignalSystem.hpp"

class Entity;

struct WinSignal : public SignalBase<WinSignal> {
    Entity *transmitter = nullptr;
    Entity *receiver = nullptr;
};

struct ResetToSpawnSignal : public SignalBase<ResetToSpawnSignal> {
    Entity* transmitter = nullptr;
};
#endif // SIGNALS_HPP

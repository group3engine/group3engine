#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include "SignalSystem.hpp"

class Entity;

struct WinSignal : public SignalBase<WinSignal> {
    Entity *transmitter = nullptr;
    Entity *receiver = nullptr;
};

struct CoinSignal : public SignalBase<CoinSignal> {
    Entity *transmitter = nullptr;
    Entity *receiver = nullptr;
};

struct ResetToSpawnSignal : public SignalBase<ResetToSpawnSignal> {
    Entity* transmitter = nullptr;
};

struct UnpauseSignal : public SignalBase<UnpauseSignal> {

};

struct CoinUncollectSignal : public SignalBase<CoinUncollectSignal> {

};
#endif // SIGNALS_HPP

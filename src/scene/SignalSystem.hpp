// Inspired by Elias Daler's Bikeshed Renderer event system
// https://github.com/eliasdaler/edbr/blob/master/edbr/include/edbr/Event/EventManager.h

#ifndef SIGNALSYSTEM_HPP
#define SIGNALSYSTEM_HPP

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

class Entity;

using SignalTypeId = uint64_t;

// Signal type count so we can increment unique signal type ids
extern SignalTypeId signalTypeCount;

// Signal
struct Signal {
    virtual ~Signal() = default;

    virtual SignalTypeId GetTypeId() const = 0;
};

template<typename T>
struct SignalBase : public Signal {
    // Static version so we can get type id when just using a template type
    static SignalTypeId TypeId() { return signalTypeId;}
    virtual SignalTypeId GetTypeId() const override { return signalTypeId; }

    // Unique signal type id incremented with exterior template
    static const SignalTypeId signalTypeId;
};

// Unique signal type id for each signal type
template<typename T>
const SignalTypeId SignalBase<T>::signalTypeId = signalTypeCount++;

// Callable
using Callable = std::function<void(Signal *)>;

// Signal system
class SignalSystem {
  public:
    // Immediately emit a signal
    void EmitSignal(Signal *signal) {
        if (auto connection = mConnections.find(signal->GetTypeId());
            connection != mConnections.end()) {
            for (auto &receiver : connection->second) {
                receiver(signal);
            }
        }
    }

    // Add a receiver callable to activate on a signal
    template<typename T, typename SignalT>
    void AddReceiver(T *receiver, void (T::*f)(SignalT *)) {
        // Create a lambda function which can be called later
        Callable callable = [receiver, f](Signal *signal) {
            // Receiver callable which takes a signal parameter
            (receiver->*f)(static_cast<SignalT *>(signal));
        };

        mConnections[SignalT::TypeId()].push_back(callable);
    }

    // TODO: Remove receiver

    // Clear connections, useful for scene switching
    void Clear() { mConnections.clear(); }

  private:
    // Connections are between event types and receiver callables
    std::unordered_map<SignalTypeId, std::vector<Callable>> mConnections;
};
#endif // SIGNALSYSTEM_HPP

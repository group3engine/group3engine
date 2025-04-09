//
// Created by thomas on 04/03/25.
//

#ifndef GROUP3ENGINE_CUSTOMCONTACTLISTENER_HPP
#define GROUP3ENGINE_CUSTOMCONTACTLISTENER_HPP
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <iostream>

class Entity;

class CustomContactListener : public JPH::ContactListener {
  private:
    std::unordered_map<JPH::BodyID, Entity *> mBodyEntityMap;
  public:
    void AddBodyEntityMapping(JPH::BodyID bodyId, Entity *entity) {
        mBodyEntityMap[bodyId] = entity;
    }

    void RemoveBodyEntityMapping(JPH::BodyID bodyId) {
        mBodyEntityMap.erase(bodyId);
    }

    std::unordered_map<JPH::BodyID, Entity *>& GetMap() {return mBodyEntityMap; };

    // See: ContactListener
    virtual JPH::ValidateResult OnContactValidate([[maybe_unused]]const JPH::Body &inBody1, [[maybe_unused]]const JPH::Body &inBody2, [[maybe_unused]]JPH::RVec3Arg inBaseOffset, [[maybe_unused]]const JPH::CollideShapeResult &inCollisionResult) override {
        // at the moment we don't care about the contact validation
        // "use the unused parameters to suppress warnings"

        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override ;
    virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override ;

    virtual void OnContactRemoved([[maybe_unused]]const JPH::SubShapeIDPair &inSubShapePair) override;
    
};

#endif // GROUP3ENGINE_CUSTOMCONTACTLISTENER_HPP

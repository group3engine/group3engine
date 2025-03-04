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

    // See: ContactListener
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override {
        // at the moment we don't care about the contact validation

        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override ;
    virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override ;

    virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {
        // we can't get the bodies that were in contact from this method (idk ask jolt)
        // so we can't call the OnCollisionEnd method of the entities
        // so we don't do anything here
    }
};

#endif // GROUP3ENGINE_CUSTOMCONTACTLISTENER_HPP

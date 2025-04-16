//
// Created by thomas on 04/03/25.
//
#include "CustomContactListener.hpp"
#include "Entity.hpp"
void CustomContactListener::OnContactAdded(const Body &inBody1, const Body &inBody2,
                                           const ContactManifold &inManifold,
                                           ContactSettings &ioSettings)
{
    // call the OnCollisionStart method of both entities
    if (mBodyEntityMap.find(inBody1.GetID()) != mBodyEntityMap.end()) {
        mBodyEntityMap[inBody1.GetID()]->OnCollisionStart(mBodyEntityMap[inBody2.GetID()]);
    }
    if (mBodyEntityMap.find(inBody2.GetID()) != mBodyEntityMap.end()) {
        mBodyEntityMap[inBody2.GetID()]->OnCollisionStart(mBodyEntityMap[inBody1.GetID()]);
    }
}
void CustomContactListener::OnContactPersisted(const Body &inBody1, const Body &inBody2,
                                               const ContactManifold &inManifold,
                                               ContactSettings &ioSettings) {

    // call the OnCollisionStay method of both entities
    if (mBodyEntityMap.find(inBody1.GetID()) != mBodyEntityMap.end()) {
        mBodyEntityMap[inBody1.GetID()]->OnCollisionStay(mBodyEntityMap[inBody2.GetID()]);
    }
    if (mBodyEntityMap.find(inBody2.GetID()) != mBodyEntityMap.end()) {
        mBodyEntityMap[inBody2.GetID()]->OnCollisionStay(mBodyEntityMap[inBody1.GetID()]);
    }
}

void CustomContactListener::OnContactRemoved(const SubShapeIDPair &inSubShapePair)
{
    const BodyID &inBody1 = inSubShapePair.GetBody1ID();
    const BodyID &inBody2 = inSubShapePair.GetBody2ID();
    // call the OnCollisionEnd method of both entities
    if (mBodyEntityMap.find(inBody1) != mBodyEntityMap.end()) {
        mBodyEntityMap[inBody1]->OnCollisionEnd(mBodyEntityMap[inBody2]);
    }
    if (mBodyEntityMap.find(inBody2) != mBodyEntityMap.end()) {
        mBodyEntityMap[inBody2]->OnCollisionEnd(mBodyEntityMap[inBody1]);
    }

}


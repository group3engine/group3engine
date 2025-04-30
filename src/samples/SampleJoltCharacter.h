// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include "CharacterBaseTest.h"
#include "CustomContactListener.hpp"

// enum of jump states - start, falling, end, none
enum class EJumpState
{
        Start,
        Falling,
        End,
        None
};

// Simple test that test the CharacterVirtual class. Allows the user to move around with the arrow keys and jump with the J button.
class SampleJoltCharacter : public CharacterBaseTest, public CharacterContactListener
{
public:
    // Initialize the test
    virtual void			Initialize() override;

    // Update the test, called before the physics update
    virtual void			PrePhysicsUpdate(const PreUpdateParams &inParams) override;

    // Called whenever the character collides with a body.
    virtual void			OnContactAdded(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings) override;

    // Called whenever the character persists colliding with a body.
    virtual void			OnContactPersisted(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings) override;

    // Called whenever the character loses contact with a body.
    virtual void			OnContactRemoved(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2) override;

    // Called whenever the character movement is solved and a constraint is hit. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
    virtual void			OnContactSolve(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial *inContactMaterial, Vec3Arg inCharacterVelocity, Vec3 &ioNewCharacterVelocity) override;

    // Get position of the character
    virtual RVec3			GetCharacterPosition() const override				{ return mCharacter->GetPosition(); }

    // Get velocity of the character (used for animation, don't include ground velocity)
    virtual Vec3			GetCharacterVelocity() const				{ return mCharacter->GetLinearVelocity() - mCharacter->GetGroundVelocity(); }
    // return if the character is grounded
    bool IsGrounded() const { return mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround; }
    // add an impulse to the character
    void AddImpulse(Vec3Arg impulse) { mAdditionalImpulse += impulse; mHasAdditionalImpulse = true; }

    [[nodiscard]] Vec3 		GetIntendedVelocity() 					{ return mIntendedVelocity; }

    // Set the character velocity
    void SetCharacterVelocity(Vec3Arg velocity) { mCharacter->SetLinearVelocity(velocity); }

    // set to manual velocity mode
    void SetManualVelocityMode(bool manual) { mManualVelocityMode = manual; }

    void SetCharacterPosition(RVec3 pos) { mCharacter->SetPosition(pos); }

    // Set the custom contact listener
    void SetCustomContactListener(CustomContactListener *inCustomContactListener) { mCustomContactListener = inCustomContactListener; }
    Ref<CharacterVirtual>	GetCharacter() {return mCharacter; }

    // get the jump state
    [[nodiscard]] EJumpState GetJumpState() { return mJumpState; }
    void SetJumpState(EJumpState state) {mJumpState = state;}

protected:
    // Common function to be called when contacts are added/persisted
    void					OnContactCommon(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings);

    // Handle user input to the character
    virtual void			HandleInput(Vec3Arg inMovementDirection, bool inJump, float inDeltaTime, bool inClimb) override;

private:
    // Character movement settings
    static inline bool		sEnableCharacterInertia = true;

    // Test configuration settings
    static inline EBackFaceMode sBackFaceMode = EBackFaceMode::CollideWithBackFaces;
    static inline float		sUpRotationX = 0;
    static inline float		sUpRotationZ = 0;
    static inline float		sMaxSlopeAngle = DegreesToRadians(45.0f);
    static inline float		sMaxStrength = 100.0f;
    static inline float		sCharacterPadding = 0.02f;
    static inline float		sPenetrationRecoverySpeed = 1.0f;
    static inline float		sPredictiveContactDistance = 0.1f;
    static inline bool		sEnableWalkStairs = true;
    static inline bool		sEnableStickToFloor = true;
    static inline bool		sEnhancedInternalEdgeRemoval = true;


    // The 'player' character
    Ref<CharacterVirtual>	mCharacter;

    // Smoothed value of the player input
    Vec3					mDesiredVelocity = Vec3::sZero();

    // desired additional impulse
    Vec3               mAdditionalImpulse = Vec3::sZero();
    bool mHasAdditionalImpulse = false;

    // True when the player is pressing movement controls
    bool					mAllowSliding = false;

    Vec3 					mIntendedVelocity {};

    // Track active contacts for debugging purposes
    using ContactSet = Array<CharacterVirtual::ContactKey>;
    ContactSet				mActiveContacts;

    CustomContactListener* mCustomContactListener;

    // current jump state
    EJumpState              mJumpState = EJumpState::None;
    // if we are in manual velocity mode
    bool                    mManualVelocityMode = false;

    float mDisplacementVertical = 0.0f;
    Vec3 mPreviousPosition;
};

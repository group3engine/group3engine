// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include "SampleJoltCharacter.h"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <spdlog/spdlog.h>

#include "PhysicsHelpers.hpp"

#include "Entity.hpp"

void SampleJoltCharacter::Initialize()
{
    CharacterBaseTest::Initialize();

    // Create 'player' character
    Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
    settings->mMaxSlopeAngle = sMaxSlopeAngle;
    settings->mMaxStrength = sMaxStrength;
    settings->mShape = mStandingShape;
    settings->mBackFaceMode = sBackFaceMode;
    settings->mCharacterPadding = sCharacterPadding;
    settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
    settings->mPredictiveContactDistance = sPredictiveContactDistance;
    settings->mSupportingVolume = Plane(Vec3::sAxisY(), -cCharacterRadiusStanding); // Accept contacts that touch the lower sphere of the capsule
    settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
    settings->mInnerBodyShape = mStandingShape;
    settings->mInnerBodyLayer = Layers::MOVING;
    Vec3 initialPosition = Vec3::sZero();
    mCharacter = new CharacterVirtual(settings, initialPosition, Quat::sIdentity(), 0, mPhysicsSystem);

    mPreviousPosition = initialPosition;

    mCharacter->SetListener(this);
}

void SampleJoltCharacter::PrePhysicsUpdate(const PreUpdateParams &inParams)
{
    CharacterBaseTest::PrePhysicsUpdate(inParams);

    // Settings for our update function
    CharacterVirtual::ExtendedUpdateSettings update_settings;
    if (!sEnableStickToFloor)
        update_settings.mStickToFloorStepDown = Vec3::sZero();
    else
        update_settings.mStickToFloorStepDown = -mCharacter->GetUp() * update_settings.mStickToFloorStepDown.Length();
    if (!sEnableWalkStairs)
        update_settings.mWalkStairsStepUp = Vec3::sZero();
    else
        update_settings.mWalkStairsStepUp = mCharacter->GetUp() * update_settings.mWalkStairsStepUp.Length();

    // Update the character position
    mCharacter->ExtendedUpdate(inParams.mDeltaTime,
        -mCharacter->GetUp() * mPhysicsSystem->GetGravity().Length(),
        update_settings,
        mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        mPhysicsSystem->GetDefaultLayerFilter(Layers::MOVING),
        { },
        { },
        *mTempAllocator);

#ifdef JPH_ENABLE_ASSERTS
    // Validate that our contact list is in sync with that of the character
    uint num_contacts = 0;
    for (const CharacterVirtual::Contact &c : mCharacter->GetActiveContacts())
        if (c.mHadCollision)
        {
            JPH_ASSERT(std::find(mActiveContacts.begin(), mActiveContacts.end(), c) != mActiveContacts.end());
            num_contacts++;
        }
    JPH_ASSERT(num_contacts == mActiveContacts.size());
#endif
}

void SampleJoltCharacter::HandleInput(Vec3Arg inMovementDirection, bool inJump, float inDeltaTime, bool inClimb)
{
    float inMovementY = inMovementDirection.GetY();
	bool player_controls_horizontal_velocity = (sControlMovementDuringJump || mCharacter->IsSupported()) && !mIsRagdolling;
    if (player_controls_horizontal_velocity)
    {
        float jumpMultiplier = mCharacter->IsSupported() ? 0.25f : 0.05f;
        // Smooth the player input
        mDesiredVelocity = sEnableCharacterInertia? jumpMultiplier * inMovementDirection * sCharacterSpeed + (1.f - jumpMultiplier) * mDesiredVelocity : inMovementDirection * sCharacterSpeed;
        mDesiredVelocity.SetY(0); // We don't want to move up/down when moving sideways
        if(inClimb && !IsGrounded())
        {
            mDesiredVelocity.SetX(0);
            mDesiredVelocity.SetZ(0);
        }

        // True if the player intended to move
        mAllowSliding = !inMovementDirection.IsNearZero();
    }
    else
    {
        // While in air we allow sliding
        mAllowSliding = true;
    }

    // Update the character rotation and its up vector to match the up vector set by the user settings
    Quat character_up_rotation = Quat::sEulerAngles(Vec3(sUpRotationX, 0, sUpRotationZ));
    mCharacter->SetUp(character_up_rotation.RotateAxisY());
    mCharacter->SetRotation(character_up_rotation);

    // A cheaper way to update the character's ground velocity,
    // the platforms that the character is standing on may have changed velocity
    mCharacter->UpdateGroundVelocity();

    Vec3 currentPosition = mCharacter->GetPosition();

    mDisplacementVertical += abs(currentPosition.GetY() - mPreviousPosition.GetY());

    // Determine new basic velocity
    Vec3 current_vertical_velocity = mCharacter->GetLinearVelocity().Dot(mCharacter->GetUp()) * mCharacter->GetUp();
    Vec3 ground_velocity = mCharacter->GetGroundVelocity();
    Vec3 new_velocity;
    bool moving_towards_ground = (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f;
    if ((mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround || inClimb)	// If on ground
        && (sEnableCharacterInertia?
            moving_towards_ground                                                   // Inertia enabled: And not moving away from ground
            : !mCharacter->IsSlopeTooSteep(mCharacter->GetGroundNormal())))         // Inertia disabled: And not on a slope that is too steep
    {
        // Assume velocity of ground when on ground
        new_velocity = ground_velocity;

        // Reset vertical displacement when grounded
        mDisplacementVertical = 0.0f;

        // Jump
        if (inJump && moving_towards_ground)
        {
            new_velocity += sJumpSpeed * mCharacter->GetUp();
            mJumpState = EJumpState::Start;
            // if we are in climb, we want to jump away from the wall, so add
            // negative of the direction without y
            if (inClimb)
            {
                Vec3 jumpBack = inMovementDirection;
                jumpBack.SetY(0);
                new_velocity += jumpBack * sJumpSpeed;
                mJumpState = EJumpState::Start;
                inClimb = false;
            }
        }
        // If we have just landed then set the jump state to the end state
        else if (mJumpState != EJumpState::None && mJumpState != EJumpState::End)
        {
            mJumpState = EJumpState::End;
        }
        // If we have already landed and are still grounded then set the jump state to none
        else
        {
            mJumpState = EJumpState::None;
        }
    }
    // If Character is not grounded
    else
    {
        new_velocity = current_vertical_velocity;

        // Since the jump is a parabola if we have started a jump measure the total vertical displacement
        // If we have gone past the peak of the parabola then we should start falling
        if (mJumpState == EJumpState::Start && mDisplacementVertical >= GetJumpHeight()) {
            mJumpState = EJumpState::Falling;
        }
    }

	// Gravity
	if (new_velocity.GetY() < 0)
	{
		new_velocity += (character_up_rotation * Vec3(0, sFallGravity, 0)) * inDeltaTime;
	} else {
		new_velocity += (character_up_rotation * Vec3(0, sJumpGravity, 0)) * inDeltaTime;
	}

    if (player_controls_horizontal_velocity)
    {
        // Player input
        new_velocity += character_up_rotation * mDesiredVelocity;
    }
    else
    {
        // Preserve horizontal velocity
        Vec3 current_horizontal_velocity = mCharacter->GetLinearVelocity() - current_vertical_velocity;
        new_velocity += current_horizontal_velocity;
    }

    // Add additional impulse
    if (mHasAdditionalImpulse)
    {
        new_velocity += mAdditionalImpulse;
        mHasAdditionalImpulse = false;
    }
    mAdditionalImpulse = Vec3::sZero();


    // if we are in climb, set the upward velocity to the magnitude of the movement direction
    if(inClimb)
    {
        new_velocity.SetY(inMovementY);
    }
    if(!mManualVelocityMode)
    {
        // Update character velocity
        mCharacter->SetLinearVelocity(new_velocity);
        mIntendedVelocity = new_velocity;
    }

    // set the shape based on the crouching and falling state
    if (mIsCrouching)
    {
        mCharacter->SetShape(mCrouchingShape, FLT_MAX, mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), mPhysicsSystem->GetDefaultLayerFilter(Layers::MOVING), { }, { }, *mTempAllocator);
        mCharacter->SetInnerBodyShape(mCrouchingShape);
    }
    else if(mJumpState == EJumpState::Falling)
    {
        mCharacter->SetShape(mFallingShape, FLT_MAX, mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), mPhysicsSystem->GetDefaultLayerFilter(Layers::MOVING), { }, { }, *mTempAllocator);
        mCharacter->SetInnerBodyShape(mFallingShape);
    }
    else
    {
        mCharacter->SetShape(mStandingShape, FLT_MAX, mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), mPhysicsSystem->GetDefaultLayerFilter(Layers::MOVING), { }, { }, *mTempAllocator);
        mCharacter->SetInnerBodyShape(mStandingShape);
    }

    mPreviousPosition = currentPosition;
}

void SampleJoltCharacter::OnContactCommon(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings)
{
    // If we encounter an object that can push the player, enable sliding
    if (inCharacter == mCharacter
        && ioSettings.mCanPushCharacter
        && mPhysicsSystem->GetBodyInterface().GetMotionType(inBodyID2) != EMotionType::Static)
        mAllowSliding = true;
}

void SampleJoltCharacter::OnContactAdded(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings)
{
    OnContactCommon(inCharacter, inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);

    if (inCharacter == mCharacter)
    {
    #ifdef CHARACTER_TRACE_CONTACTS
        Trace("Contact added with body %08x, sub shape %08x", inBodyID2.GetIndexAndSequenceNumber(), inSubShapeID2.GetValue());
    #endif
        CharacterVirtual::ContactKey c(inBodyID2, inSubShapeID2);
        if (std::find(mActiveContacts.begin(), mActiveContacts.end(), c) != mActiveContacts.end()) {
            SPDLOG_ERROR("Got an add contact that should have been a persisted contact");
			exit(EXIT_FAILURE);
        }

        // assume the thing isnt already an active contact
        bool is_already_in_contact = false;

        // for all contacts
        for(auto contact: mActiveContacts)
        {
            // if the new contact has the same body as one that already exists
            if(c.IsSameBody(contact))
            {
                // then its already in contact
                is_already_in_contact = true;
            }
        }

        // we push this contact into the list of active contacts
        mActiveContacts.push_back(c);

        // if its not already in contact
        if(!is_already_in_contact)
        {
            // handle the contact
            if(mCustomContactListener->GetMap().find(inBodyID2) != mCustomContactListener->GetMap().end()) {
                mCustomContactListener->GetMap()[inBodyID2]->OnCollisionStart(mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]);
            }

            if(mCustomContactListener->GetMap().find(inCharacter->GetInnerBodyID()) != mCustomContactListener->GetMap().end()) {
                mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]->OnCollisionStart(mCustomContactListener->GetMap()[inBodyID2]);
            }
        }
    }
}

void SampleJoltCharacter::OnContactPersisted(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings)
{
    OnContactCommon(inCharacter, inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);

    if (inCharacter == mCharacter)
    {
    #ifdef CHARACTER_TRACE_CONTACTS
        Trace("Contact persisted with body %08x, sub shape %08x", inBodyID2.GetIndexAndSequenceNumber(), inSubShapeID2.GetValue());
    #endif
        if (std::find(mActiveContacts.begin(), mActiveContacts.end(), CharacterVirtual::ContactKey(inBodyID2, inSubShapeID2)) == mActiveContacts.end()) {
            SPDLOG_ERROR("Got a persisted contact that should have been an add contact");
			exit(EXIT_FAILURE);
        }

        // handle the contact
        if(mCustomContactListener->GetMap().find(inBodyID2) != mCustomContactListener->GetMap().end()) {
            mCustomContactListener->GetMap()[inBodyID2]->OnCollisionStay(mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]);
        }

        if(mCustomContactListener->GetMap().find(inCharacter->GetInnerBodyID()) != mCustomContactListener->GetMap().end()) {
            mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]->OnCollisionStay(mCustomContactListener->GetMap()[inBodyID2]);
        }
    }
}

void SampleJoltCharacter::OnContactRemoved(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2)
{
    if (inCharacter == mCharacter)
    {
    #ifdef CHARACTER_TRACE_CONTACTS
        Trace("Contact removed with body %08x, sub shape %08x", inBodyID2.GetIndexAndSequenceNumber(), inSubShapeID2.GetValue());
    #endif
        ContactSet::iterator it = std::find(mActiveContacts.begin(), mActiveContacts.end(), CharacterVirtual::ContactKey(inBodyID2, inSubShapeID2));
        if (it == mActiveContacts.end()) {
            SPDLOG_ERROR("Got a remove contact that has not been added");
			exit(EXIT_FAILURE);
        }
        mActiveContacts.erase(it);

        // handle the contact
        if(mCustomContactListener->GetMap().find(inBodyID2) != mCustomContactListener->GetMap().end()) {
            mCustomContactListener->GetMap()[inBodyID2]->OnCollisionEnd(mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]);
        }
        if(mCustomContactListener->GetMap().find(inCharacter->GetInnerBodyID()) != mCustomContactListener->GetMap().end()) {
            mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]->OnCollisionEnd(mCustomContactListener->GetMap()[inBodyID2]);
        }
    }

}

void SampleJoltCharacter::OnContactSolve(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial *inContactMaterial, Vec3Arg inCharacterVelocity, Vec3 &ioNewCharacterVelocity)
{
    // Ignore callbacks for other characters than the player
    if (inCharacter != mCharacter)
        return;

    // Don't allow the player to slide down static not-too-steep surfaces when not actively moving and when not on a moving platform
    if (!mAllowSliding && inContactVelocity.IsNearZero() && !inCharacter->IsSlopeTooSteep(inContactNormal))
        ioNewCharacterVelocity = Vec3::sZero();
}
void SampleJoltCharacter::SetCharacterImpulse(Vec3 impulse) {
    // get the current velocity
    Vec3 current_velocity = mCharacter->GetLinearVelocity();
    // add the impulse, divide by mass
    current_velocity += impulse / mCharacter->GetMass();
    // set the new velocity
    mCharacter->SetLinearVelocity(current_velocity);
}

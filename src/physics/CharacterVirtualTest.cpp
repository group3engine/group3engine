// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include "CharacterVirtualTest.h"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <spdlog/spdlog.h>

#include "PhysicsHelpers.hpp"

#include "Entity.hpp"

void CharacterVirtualTest::Initialize()
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
	mCharacter = new CharacterVirtual(settings, RVec3::sZero(), Quat::sIdentity(), 0, mPhysicsSystem);

	mCharacter->SetListener(this);
}

void CharacterVirtualTest::PrePhysicsUpdate(const PreUpdateParams &inParams)
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

void CharacterVirtualTest::HandleInput(Vec3Arg inMovementDirection, bool inJump, float inDeltaTime)
{
	bool player_controls_horizontal_velocity = sControlMovementDuringJump || mCharacter->IsSupported();
	if (player_controls_horizontal_velocity)
	{
		// Smooth the player input
		mDesiredVelocity = sEnableCharacterInertia? 0.25f * inMovementDirection * sCharacterSpeed + 0.75f * mDesiredVelocity : inMovementDirection * sCharacterSpeed;

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

	// Determine new basic velocity
	Vec3 current_vertical_velocity = mCharacter->GetLinearVelocity().Dot(mCharacter->GetUp()) * mCharacter->GetUp();
	Vec3 ground_velocity = mCharacter->GetGroundVelocity();
	Vec3 new_velocity;
	bool moving_towards_ground = (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f;
	if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround	// If on ground
		&& (sEnableCharacterInertia?
			moving_towards_ground													// Inertia enabled: And not moving away from ground
			: !mCharacter->IsSlopeTooSteep(mCharacter->GetGroundNormal())))			// Inertia disabled: And not on a slope that is too steep
	{
		// Assume velocity of ground when on ground
		new_velocity = ground_velocity;

		// Jump
		if (inJump && moving_towards_ground) {
                    new_velocity += sJumpSpeed * mCharacter->GetUp();
                        mJumpState = EJumpState::Start;
                }
                else if (mJumpState != EJumpState::None && mJumpState != EJumpState::End) {
                    mJumpState = EJumpState::End;
                }
                else
                {
                        mJumpState = EJumpState::None;
                }
	}
	else {
            new_velocity = current_vertical_velocity;
            mJumpState = EJumpState::Falling;
        }

	// Gravity
	new_velocity += (character_up_rotation * mPhysicsSystem->GetGravity()) * inDeltaTime;

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

	// Update character velocity
	mCharacter->SetLinearVelocity(new_velocity);
}

void CharacterVirtualTest::OnContactCommon(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings)
{
	// If we encounter an object that can push the player, enable sliding
	if (inCharacter == mCharacter
		&& ioSettings.mCanPushCharacter
		&& mPhysicsSystem->GetBodyInterface().GetMotionType(inBodyID2) != EMotionType::Static)
		mAllowSliding = true;
}

void CharacterVirtualTest::OnContactAdded(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings)
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
		mActiveContacts.push_back(c);
	}

    if(mCustomContactListener->GetMap().find(inBodyID2) != mCustomContactListener->GetMap().end()) {
        mCustomContactListener->GetMap()[inBodyID2]->OnCollisionStart(mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]);
    }

    if(mCustomContactListener->GetMap().find(inCharacter->GetInnerBodyID()) != mCustomContactListener->GetMap().end()) {
        mCustomContactListener->GetMap()[inCharacter->GetInnerBodyID()]->OnCollisionStart(mCustomContactListener->GetMap()[inBodyID2]);
    }
}

void CharacterVirtualTest::OnContactPersisted(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings)
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
	}
}

void CharacterVirtualTest::OnContactRemoved(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2)
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
	}
}

void CharacterVirtualTest::OnContactSolve(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial *inContactMaterial, Vec3Arg inCharacterVelocity, Vec3 &ioNewCharacterVelocity)
{
	// Ignore callbacks for other characters than the player
	if (inCharacter != mCharacter)
		return;

	// Don't allow the player to slide down static not-too-steep surfaces when not actively moving and when not on a moving platform
	if (!mAllowSliding && inContactVelocity.IsNearZero() && !inCharacter->IsSlopeTooSteep(inContactNormal))
		ioNewCharacterVelocity = Vec3::sZero();
}

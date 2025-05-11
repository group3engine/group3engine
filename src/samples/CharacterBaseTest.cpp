// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include "CharacterBaseTest.h"

#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Core/StringTools.h>

#include "../core/Input.hpp"

#include "../physics/PhysicsHelpers.hpp"

#include "Camera.hpp"

using namespace JPH;

// Scene constants
static const RVec3 cCharacterVirtualPosition(-5.0f, 0, 3.0f);

void CharacterBaseTest::Initialize()
{
	// Create capsule shapes for all stances
	switch (sShapeType)
	{
	case EType::Capsule:
		mStandingShape = RotatedTranslatedShapeSettings(Vec3(0, cCharacterHeightStanding, 0), Quat::sIdentity(), new CapsuleShape(cCharacterHeightStanding, cCharacterRadiusStanding)).Create().Get();
        mCrouchingShape = RotatedTranslatedShapeSettings(Vec3(0, cCharacterHeightCrouching, 0), Quat::sIdentity(), new CapsuleShape(cCharacterHeightCrouching, cCharacterRadiusStanding)).Create().Get();
        mFallingShape = RotatedTranslatedShapeSettings(Vec3(0, cCharacterHeightFalling, 0), Quat::sIdentity(), new CapsuleShape(cCharacterHeightFalling, cCharacterRadiusStanding)).Create().Get();
		break;
        default:
            assert(false);
	}

	// Create CharacterVirtual
	{
		CharacterVirtualSettings settings;
		settings.mShape = mStandingShape;
		settings.mSupportingVolume = Plane(Vec3::sAxisY(), -cCharacterRadiusStanding); // Accept contacts that touch the lower sphere of the capsule
		mAnimatedCharacterVirtual = new CharacterVirtual(&settings, cCharacterVirtualPosition, Quat::sIdentity(), 0, mPhysicsSystem);
	}
}

void CharacterBaseTest::ProcessInput(glm::vec3 controlInput, bool jump, bool inClimb, bool isCrouching)

{
        mJump = jump;
        mInClimb = inClimb;
        mControlInput = Vec3(controlInput.x, controlInput.y, controlInput.z);
        mIsCrouching = isCrouching;
}

void CharacterBaseTest::PrePhysicsUpdate(const PreUpdateParams &inParams)
{
	// Update scene time
	mTime += inParams.mDeltaTime;

	// Update camera pivot
	mCameraPivot = GetCharacterPosition();

	// Animate character virtual
	CharacterVirtual *character = mAnimatedCharacterVirtual;
	if (character != nullptr)
	{
		// Update velocity and apply gravity
		Vec3 velocity;
		if (character->GetGroundState() == CharacterVirtual::EGroundState::OnGround)
			velocity = Vec3::sZero();
		else
			velocity = character->GetLinearVelocity() * character->GetUp() + mPhysicsSystem->GetGravity() * inParams.mDeltaTime;
		character->SetLinearVelocity(velocity);

		// Move character
		CharacterVirtual::ExtendedUpdateSettings update_settings;
		character->ExtendedUpdate(inParams.mDeltaTime,
			mPhysicsSystem->GetGravity(),
			update_settings,
			mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::PLAYER),
			mPhysicsSystem->GetDefaultLayerFilter(Layers::PLAYER),
			{ },
			{ },
			*mTempAllocator);
	}



	// Call handle input after new velocities have been set to avoid frame delay
	HandleInput(mControlInput, mJump, inParams.mDeltaTime, mInClimb);
}

void CharacterBaseTest::GetInitialCamera(CameraState& ioState) const
{
	// This will become the local space offset, look down the x axis and slightly down
	ioState.mPos = RVec3::sZero();
	ioState.mForward = Vec3(10.0f, -2.0f, 0).Normalized();
}

RMat44 CharacterBaseTest::GetCameraPivot(float inCameraHeading, float inCameraPitch) const
{
	// Pivot is center of character + distance behind based on the heading and pitch of the camera
	Vec3 fwd = Vec3(Cos(inCameraPitch) * Cos(inCameraHeading), Sin(inCameraPitch), Cos(inCameraPitch) * Sin(inCameraHeading));
	return RMat44::sTranslation(mCameraPivot + Vec3(0, cCharacterHeightStanding + cCharacterRadiusStanding, 0) - 5.0f * fwd);
}

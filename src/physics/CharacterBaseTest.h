// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include "Input.hpp"

using namespace JPH;

struct CameraState
{
									CameraState() : mPos(RVec3::sZero()), mForward(0, 0, -1), mUp(0, 1, 0), mFOVY(DegreesToRadians(70.0f)) { }

	RVec3							mPos;								///< Camera position
	Vec3							mForward;							///< Camera forward vector
	Vec3							mUp;								///< Camera up vector
	float							mFOVY;								///< Field of view in radians in up direction
};

class ProcessInputParams
{
public:
	float								mDeltaTime;
	CameraState							mCameraState;
};

class PreUpdateParams
{
public:
	float								mDeltaTime;
	CameraState							mCameraState;
};

class CharacterBaseTest
{
public:
	virtual ~CharacterBaseTest() = default;

	// Initialize the test
	virtual void 			Initialize();

	// Process input
	void					ProcessInput(const ProcessInputParams &inParams);

	// Update the test, called before the physics update
	virtual void			PrePhysicsUpdate(const PreUpdateParams &inParams);

	// Override to specify the initial camera state (local to GetCameraPivot)
	virtual void			GetInitialCamera(CameraState &ioState) const;

	// Override to specify a camera pivot point and orientation (world space)
	virtual RMat44			GetCameraPivot(float inCameraHeading, float inCameraPitch) const;

	// Set the physics system
	virtual void	SetPhysicsSystem(PhysicsSystem *inPhysicsSystem)			{ mPhysicsSystem = inPhysicsSystem; mBodyInterface = &inPhysicsSystem->GetBodyInterface(); }

	// Set the job system
	void			SetJobSystem(JobSystem *inJobSystem)						{ mJobSystem = inJobSystem; }

	// Set the temp allocator
	void			SetTempAllocator(TempAllocator *inTempAllocator)			{ mTempAllocator = inTempAllocator; }

public:
	// Get position of the character
	virtual RVec3			GetCharacterPosition() const = 0;

	// Handle user input to the character
	virtual void			HandleInput(Vec3Arg inMovementDirection, bool inJump, float inDeltaTime) = 0;

	// Character size
	static constexpr float	cCharacterHeightStanding = 1.35f;
	static constexpr float	cCharacterRadiusStanding = 0.3f;

	// Character movement properties
	inline static bool		sControlMovementDuringJump = true;							///< If false the character cannot change movement direction in mid air
	inline static float		sCharacterSpeed = 6.0f;
	inline static float		sJumpSpeed = 4.0f;

	// The different stances for the character
	RefConst<Shape>			mStandingShape;

	JobSystem *		mJobSystem = nullptr;
	PhysicsSystem *	mPhysicsSystem = nullptr;
	BodyInterface *	mBodyInterface = nullptr;
	TempAllocator *	mTempAllocator = nullptr;

private:
	// Shape types
	enum class EType
	{
		Capsule,
		Cylinder,
		Box
	};

	// Character shape type
	static inline EType		sShapeType = EType::Box;

	// Scene time (for moving bodies)
	float					mTime = 0.0f;

	// The camera pivot, recorded before the physics update to align with the drawn world
	RVec3					mCameraPivot = RVec3::sZero();

	// Moving characters
	Ref<CharacterVirtual>	mAnimatedCharacterVirtual;

	// Player input
	Vec3					mControlInput = Vec3::sZero();
	bool					mJump = false;
	bool					mWasJump = false;
};

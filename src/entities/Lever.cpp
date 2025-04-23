#include "Lever.hpp"

#include <stack>

#include "Input.hpp"

Lever::Lever() {
    mType = "lever";
}

void Lever::Awake() {
    // Lever must be the parent of a lever handle and anything that it controls

    auto Execute = [this](Entity *entity){
        if (entity->CompareTag("lever_handle")) {
            mLeverHandle = entity;
        } else if (entity->CompareType("trapdoor")) {
            mTrapdoors.push_back(static_cast<Trapdoor*>(entity));
        }
    };

    // Find lever handle and trap door
    std::stack<Entity *> stack;
    stack.push(this);
    // DFS with execute payload
    while (!stack.empty()) {
        auto *entity = stack.top();
        stack.pop();

        for (auto *child : entity->GetChildren()) {
            Execute(child);
            stack.push(child);
        }
    }
    assert(mLeverHandle);
    assert(!mTrapdoors.empty());

    const auto &leverHandleProperties = mLeverHandle->GetFloatProperties();
    auto minAngle = leverHandleProperties.find("min_angle");
    auto maxAngle = leverHandleProperties.find("max_angle");
    auto animationTime = leverHandleProperties.find("animation_time");
    // Find the minimum and maximum angle of the lever handle rotation
    if (minAngle != leverHandleProperties.end() &&
        maxAngle != leverHandleProperties.end() &&
        animationTime != leverHandleProperties.end()) {
        // Set float properties
        mMinAngle = JPH::DegreesToRadians(minAngle->second);
        mMaxAngle = JPH::DegreesToRadians(maxAngle->second);
        mAnimationTime = animationTime->second;
    } else {
        SPDLOG_ERROR("Lever handle missing required property.");
        exit(EXIT_FAILURE);
    }

    // Scoped lock
    {
        JPH::BodyLockRead lock(mLockInterface, mLeverHandle->GetRigidBody().mBodyId);

        const JPH::Body &leverBody = lock.GetBody();

        mAxisX = leverBody.GetWorldTransform().GetAxisX().Normalized();
        mAxisY = leverBody.GetWorldTransform().GetAxisY().Normalized();
        mAxisZ = leverBody.GetWorldTransform().GetAxisZ().Normalized();

        mInitialRotation = JPH::Quat::sRotation(mAxisZ, mMinAngle) * leverBody.GetRotation();
        mFinalRotation = JPH::Quat::sRotation(mAxisZ, mMaxAngle) * leverBody.GetRotation();
    }

    mLeverHandle->GetRigidBody().SetRotationJolt(mInitialRotation);
}

void Lever::Update(double deltaTime) {
    // TODO: Refactor trapdoor so that the class finds its children automatically
    auto trapdoorsActivated =
        std::any_of(mTrapdoors.begin(), mTrapdoors.end(), [](auto *e) { return e->IsActivated(); });

    // TODO: Add key binding and factor out input checking into the character
    // TODO: Add E to interact UI
    // Send an event from the character?
    if (!mIsPulled && !trapdoorsActivated && IsKeyPressed(KEY::eE)) {
        mIsPulled = true;

        for (auto *trapdoor : mTrapdoors) {
            trapdoor->Activate();
        }
    }

    if (mCurrentAnimationTime >= mAnimationTime) {
        // TODO: Refactor this code duplication? If the current animation time has gone past the
        // total animation time then the last part of the animation needs to finish
        float fraction = JPH::Clamp(mCurrentAnimationTime / mAnimationTime, 0.0f, 1.0f);
        JPH::Quat rotation = mInitialRotation.SLERP(mFinalRotation, fraction);
        mLeverHandle->GetRigidBody().SetRotationJolt(rotation.Normalized());

        mIsPulled = false;
        mCurrentAnimationTime = 0.0f;
        // Swap between initial and final rotation so the lever can be switched back and forth
        std::swap(mInitialRotation, mFinalRotation);
    }

    if (mIsPulled) {
        float fraction = JPH::Clamp(mCurrentAnimationTime / mAnimationTime, 0.0f, 1.0f);
        JPH::Quat rotation = mInitialRotation.SLERP(mFinalRotation, fraction);
        mLeverHandle->GetRigidBody().SetRotationJolt(rotation.Normalized());

        mCurrentAnimationTime += deltaTime;
    }
}

//
// Created by thomas on 17/04/25.
//

#include "SpikeTrap.hpp"

void SpikeTrap::Awake()
{
    initialRotation = GetLocalTransform().rotation;
    // hit rotation is initial rotation + 90 degrees around the y axis
    hitRotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * initialRotation;
}

void SpikeTrap::Update(double aDeltaTime)
{
    timer += aDeltaTime;
    switch(mState)
    {
        case SpikeTrapState::eWaiting:
            // keep the spike trap in the initial position
            {
                Transform transform = GetLocalTransform();
                transform.rotation = initialRotation;
                SetTransform(transform);
            }
            if(timer >= waitTime)
            {
                timer = 0.0;
                mState = SpikeTrapState::eHitting;
            }
            break;
        case SpikeTrapState::eHitting:
            // rotate the spike trap down (slerp between the initial and hit rotation)
            {
                float t = timer / hittingTime;
                glm::quat rotation = glm::slerp(initialRotation, hitRotation, t);
                Transform transform = GetLocalTransform();
                transform.rotation = rotation;
                SetTransform(transform);
            }
            if(timer >= hittingTime)
            {
                timer = 0.0;
                mState = SpikeTrapState::eHit;
            }
            break;
        case SpikeTrapState::eHit:
            // keep the spike trap in the hit position
            {
                Transform transform = GetLocalTransform();
                transform.rotation = hitRotation;
                SetTransform(transform);
            }
            if(timer >= hitTime)
            {
                timer = 0.0;
                mState = SpikeTrapState::eRetracting;
            }
            break;
        case SpikeTrapState::eRetracting:
            // rotate the spike trap up (slerp between the hit and initial rotation)
            {
                float t = timer / retractTime;
                glm::quat rotation = glm::slerp(hitRotation, initialRotation, t);
                Transform transform = GetLocalTransform();
                transform.rotation = rotation;
                SetTransform(transform);
            }
            if(timer >= retractTime)
            {
                timer = 0.0;
                mState = SpikeTrapState::eWaiting;
            }
            break;

    }
}

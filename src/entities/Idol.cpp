#include "Idol.hpp"

#include "Scene.hpp"
#include "SignalSystem.hpp"
#include "Signals.hpp"

Idol::Idol() {
    mType = "idol";
}

void Idol::Awake() {
    // Find float properties
    auto proximityPrompt = mFloatProperties.find("proximity_prompt");
    if (proximityPrompt != mFloatProperties.end()) {
        mProximityPromptRadius = proximityPrompt->second;
    } else {
        SPDLOG_ERROR("Idol missing required property.");
        exit(EXIT_FAILURE);
    }

    // Init proximity prompt sensor
    // TODO: Factor this out into a class so we can programatically create proximity sensors given a
    // proximity_prompt float property
    glm::vec3 translation = GetParent()->GetWorldTransformComponents().translation;
    BodyCreationSettings sensorSettings(new SphereShape(mProximityPromptRadius),
                                        Vec3(translation.x, translation.y, translation.z),
                                        Quat::sIdentity(), EMotionType::Static, Layers::MOVING);
    SPDLOG_INFO("Idol position {}", glm::to_string(translation));
    sensorSettings.mIsSensor = true;

    mSensor = std::make_unique<RigidBody>(sensorSettings);

    mSensor->Init(PhysicsManager::get(), false);
    PhysicsManager::get().RegisterEntity(this, mSensor->mBodyId);

    GetScene()->mSignalSystem.AddReceiver<Idol, ResetToSpawnSignal>(this, &Idol::OnResetToSpawn);
    // get the animator and play idol hover
    GetAnimator().SetActiveAnimation("idolhover", 0, false, true);
}

void Idol::OnInteract(Entity *other, ENetworkLocality networkLocality) {
    if (!mIsCollected) {
        mIsCollected = true;

        WinSignal winSignal = {};
        winSignal.transmitter = this;
        winSignal.receiver = other;
        GetScene()->mSignalSystem.EmitSignal(&winSignal);

        SetAsInvisible();
    }
}

void Idol::OnResetToSpawn([[maybe_unused]] ResetToSpawnSignal *signal) {
    mIsCollected = false;
    SetAsVisible();
}

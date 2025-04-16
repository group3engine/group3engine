#ifndef PHYSICS_PHYSICSMANAGER_HPP
#define PHYSICS_PHYSICSMANAGER_HPP

#include <memory>
#include <vector>

#include "PhysicsHelpers.hpp"
#include "CustomContactListener.hpp"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH /
// JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value
// that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// This is the max amount of rigid bodies that you can add to the physics system. If you try to add
// more you'll get an error. Note: This value is low because this is a simple test. For a real
// project use something in the order of 65536.
const uint cMaxBodies = 1024;

// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set
// it to 0 for the default settings.
const uint cNumBodyMutexes = 0;

// This is the max amount of body pairs that can be queued at any time (the broad phase will detect
// overlapping body pairs based on their bounding boxes and will insert them into a queue for the
// narrowphase). If you make this buffer too small the queue will fill up and the broad phase jobs
// will start to do narrow phase work. This is slightly less efficient. Note: This value is low
// because this is a simple test. For a real project use something in the order of 65536.
const uint cMaxBodyPairs = 1024;

// This is the maximum size of the contact constraint buffer. If more contacts (collisions between
// bodies) are detected than this number then these contacts will be ignored and bodies will start
// interpenetrating / fall through the world. Note: This value is low because this is a simple test.
// For a real project use something in the order of 10240.
const uint cMaxContactConstraints = 1024;

const int ten_megabytes = 10 * 1024 * 1024;



/// The PhysicsManager class is a singleton that manages the physics system and the bodies in it.
class PhysicsManager {
  private:
    PhysicsManager() = default;
    ~PhysicsManager() = default;

  public:
    PhysicsManager(const PhysicsManager &) = delete;
    PhysicsManager &operator=(const PhysicsManager &) = delete;

    /// Get the singleton instance of the PhysicsManager
    static PhysicsManager &get() {
        static PhysicsManager instance;
        return instance;
    }

    // Per frame update function
    void UpdatePhysics(double delta_time);

    void StartUp();
    void ShutDown();

    /// registers a entity as a rigid bodies owner
    void RegisterEntity(Entity *entity, BodyID bodyId);

    /// Unregister a body from a body-entity mapping
    void UnregisterBody(BodyID bodyId);

    /// Remove and destroy body from the physics system
    void RemoveAndDestroyBody(BodyID bodyId);

  public:
    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    std::unique_ptr<TempAllocatorImpl> mTempAllocator;
    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    std::unique_ptr<JobSystemThreadPool> mJobSystem;

    Factory sInstance;

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance
    // needs to stay alive! Also have a look at BroadPhaseLayerInterfaceTable or
    // BroadPhaseLayerInterfaceMask for a simpler interface.
    BPLayerInterfaceImpl mBroadPhaseLayerInterface;

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance
    // needs to stay alive! Also have a look at ObjectVsBroadPhaseLayerFilterTable or
    // ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
    ObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadphaseLayerFilter;

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance
    // needs to stay alive! Also have a look at ObjectLayerPairFilterTable or
    // ObjectLayerPairFilterMask for a simpler interface.
    ObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;

    PhysicsSystem mPhysicsSystem;
    MyBodyActivationListener mBodyActivationListener;
    CustomContactListener mContactListener;
    BodyInterface &mBodyInterface = mPhysicsSystem.GetBodyInterface();

    float cDeltaTime = 1.0f / 60.0f;

    std::vector<BodyID> mBodyIds;

};
#endif // PHYSICS_PHYSICSMANAGER_HPP

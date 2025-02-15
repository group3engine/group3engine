#ifndef HELLOWORLD_HPP
#define HELLOWORLD_HPP

#include "PhysicsManager.hpp"
#include "RigidBody.hpp"
// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;


// Program entry point
inline void HelloWorld()
{
    // make the manager
    PhysicsManager manager = PhysicsManager();

    // make settings to create the sphere
    BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    
    // add the sphere to the physics system
    RigidBody ball = RigidBody(sphere_settings, &manager);

    // add linear velocity to the ball
    manager.body_interface.SetLinearVelocity(ball.ID, Vec3(0.0f, 5.0f, 0.0f));
    
    // add the floor using the default constructor
    RigidBody floor = RigidBody(RigidBody::Floor, &manager);

    // loop until the ball goes to sleep (comes to a stop)
    while(manager.body_interface.IsActive(ball.ID))
    {
        manager.UpdatePhysics();
        ball.GetPosition();
        ball.GetVelocity();
    }
}
#endif // HELLOWORLD_HPP

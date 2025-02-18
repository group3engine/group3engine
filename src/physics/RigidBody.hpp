#ifndef PHYSICS_RIGIDBODY_HPP
#define PHYSICS_RIGIDBODY_HPP

#include <glm/glm.hpp>
#include "PhysicsManager.hpp"
#include "glm/fwd.hpp"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

class RigidBody
{
    
    public:
        // enumerations for default test objects
        enum Shape {Ball, Floor};

        // its ID in the physics manager body interface system
        BodyID ID;

        // Constructor
        RigidBody(Shape input_shape); // test object constructor
        RigidBody(BodyCreationSettings settings); // custom object constructor

        // Functions
        glm::vec4 GetPosition();
        glm::vec4 GetVelocity();
        glm::mat4 GetWorldTransform();


};

#endif
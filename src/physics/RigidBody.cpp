#include "RigidBody.hpp"
#include "glm/detail/qualifier.hpp"
#include "glm/fwd.hpp"
#include "glm/ext.hpp"
#include "spdlog/spdlog.h"

#include "PhysicsManager.hpp"

size_t gRigidBodyCounter = 0;

// RigidBody()
// input: enum default shape (Ball, Floor), a physics manager
// output: the RigidBody object
// action: creates the object according to the default test shape configuration
RigidBody::RigidBody(Shape input_shape)
{
    spdlog::info("Adding RigidBody {}", gRigidBodyCounter++);

    if(input_shape == Floor)
    {
        // Next we can create a rigid body to serve as the floor, we make a large box
        // Create the settings for the collision volume (the shape).
        // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
        BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));
        floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

        // Create the shape
        ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
        ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

        // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
        BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
        floor_settings.mRestitution = 0.5f;
        // Create the actual rigid body
        ID = PhysicsManager::get().physics_system.GetBodyInterface().CreateAndAddBody(floor_settings, EActivation::DontActivate); // Note that if we run out of bodies this can return nullptr

        PhysicsManager::get().body_ids.push_back(ID);
    }
    else if(input_shape == Ball)
    {
        // Now create a dynamic body to bounce on the floor
        // Note that this uses the shorthand version of creating and adding a body to the world
        BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
        ID = PhysicsManager::get().physics_system.GetBodyInterface().CreateAndAddBody(sphere_settings, EActivation::Activate);
        // Now you can interact with the dynamic body, in this case we're going to give it a velocity.
        // (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
        PhysicsManager::get().physics_system.GetBodyInterface().SetLinearVelocity(ID, Vec3(0.0f, 5.0f, 0.0f));
        PhysicsManager::get().body_ids.push_back(ID);
    }
    
}

// RigidBody()
// input: Jolt BodyCreationSettings, a physics manager
// output: the RigidBody object
// action: creates the object according to the body creation settings given
// TODO: potentially change the input such that it takes in a struct containing BodyCreationSettings, initial activation mode and starting linear velocity (+ other state stuff)
RigidBody::RigidBody(BodyCreationSettings settings)
{
    spdlog::info("Adding RigidBody {}", gRigidBodyCounter++);

    // use the settings to create a body and add it to the system
    ID = PhysicsManager::get().physics_system.GetBodyInterface().CreateAndAddBody(settings, EActivation::Activate);

    // add it to the managers list of IDS
    PhysicsManager::get().body_ids.push_back(ID);
}

// GetPosition()
// input: none
// output: glm vec4 of the position
glm::vec4 RigidBody::GetPosition()
{
    RVec3 position = PhysicsManager::get().physics_system.GetBodyInterface().GetCenterOfMassPosition(PhysicsManager::get().body_ids[0]);

    spdlog::info("renderer body id {} address {}", ID.GetIndex(), reinterpret_cast<size_t>(&ID));

    // we are returning a point so the w componnet is 1.0
    glm::vec4 return_position = glm::vec4(position.GetX(), position.GetY(), position.GetZ(), 1.f);

    spdlog::info("renderer pos {}", glm::to_string(return_position));

    return return_position;
}

// GetVelocity()
// input: none
// output: glm vec4 of the velocity
glm::vec4 RigidBody::GetVelocity()
{
    Vec3 velocity = PhysicsManager::get().physics_system.GetBodyInterface().GetLinearVelocity(PhysicsManager::get().body_ids[0]);

    // we are returning a vector so the w componnet is 0.0
    glm::vec4 return_velocity = glm::vec4(velocity.GetX(), velocity.GetY(), velocity.GetZ(), 0.f);

    return return_velocity;
}

glm::mat4 RigidBody::GetWorldTransform()
{
    RMat44 world_transform = PhysicsManager::get().physics_system.GetBodyInterface().GetWorldTransform(PhysicsManager::get().body_ids[0]);

    glm::mat4 return_world_transform;
    for(int row = 0; row < 4; row++)
    {
        for(int column = 0; column < 4; column++)
        {
            return_world_transform[row][column] = world_transform(row,column);
        }
    }
    

    return return_world_transform;

}
//
// Created by thomas on 22/03/25.
//

#include "ParticleSystem.hpp"

#include "spdlog/spdlog-inl.h"


ParticleSystem::ParticleSystem(Context &aContext, ParticleSystemSettings aSettings) : mSettings(mSettings)
{
    // get the number of particles from the settings
    int maxParticles = mSettings.maxParticles;
    // allocate the particles array
    mParticles = new Particle[maxParticles];
    // TODO: change the buffer allocation bits to be device only, we want to be updating this only in compute shaders
    mParticlesBuffer = CreateBuffer("particle system buffer", aContext, sizeof(Particle) * maxParticles, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                  VMA_ALLOCATION_CREATE_MAPPED_BIT);
    // TODO: create the descriptor set
    // TODO: create the pipeline
}

ParticleSystem::~ParticleSystem()
{
    // delete the particles array
    delete[] mParticles;
}


Emission SphereParticleSpawn(int &seed, Shapeconst &sphereShape)
{
    Emission returnValue{};
    do
    {
        // randomly generate a spawn position
        glm::vec3 spawnPosition = {get_random(seed++), get_random(seed++), get_random(seed++)};
        if (glm::length2(spawnPosition) <= 1.f)
        {
            // if the emit type is surphace, normalise the spawn position to put it on the surface
            if (sphereShape.box.emitFrom == EmitFrom::Surface)
            {
                spawnPosition = glm::normalize(spawnPosition);
                returnValue.Velocity = spawnPosition;
            }
            else
            {
                returnValue.Velocity = glm::normalize(spawnPosition);
            }
            spawnPosition *= sphereShape.sphere.radius;
            returnValue.Position = spawnPosition;
            return returnValue;
        }


    }while (true);
    assert(false);
}
Emission BoxParticleSpawn(int &seed, Shape const &boxShape)
{
    assert(boxShape.box.emitFrom == EmitFrom::Volume);
    Emission returnValue{};
    // randomly generate a spawn position
    returnValue.Position = {get_random(seed++) * boxShape.box.size.x, get_random(seed++) * boxShape.box.size.y, get_random(seed++) * boxShape.box.size.z};
    returnValue.Velocity = glm::normalize(returnValue.Position);
    return returnValue;
}

Emission ConeParticleSpawn(int &seed, Shape const &coneShape)
{
    // get the radius of the cone at the top - the bottom radius + the height * tan(angle)
    float tanTheta = glm::tan(coneShape.cone.angle);
    float topRadius = coneShape.cone.radius + coneShape.cone.height * tanTheta;
    Emission returnValue{};
    do
    {
        // randomly generate a spawn position, x and z are between -radius and radius, y is between 0 and height
        glm::vec3 spawnPosition = {get_random(seed++) * topRadius, get_hash(seed++) * coneShape.cone.height, get_random(seed++) * topRadius};
        // if the point is inside the cone
        // work out the radius at the height of the point
        float radiusAtHeight = coneShape.cone.radius + spawnPosition.y * tanTheta;
        // if the value of x or z is larger than the radius at the height, the point is outside the cone
        if (glm::abs(spawnPosition.x) <= radiusAtHeight && glm::abs(spawnPosition.z) <= radiusAtHeight)
        {
            // if the emit type is surface, we don't support this yet
            if (coneShape.cone.emitFrom == EmitFrom::Surface)
            {
                assert(false);
            }
            else
            {
                returnValue.Velocity = glm::normalize(spawnPosition);
            }
            returnValue.Position = spawnPosition;
            return returnValue;
        }
    }while (true);
}


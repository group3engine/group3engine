//
// Created by thomas on 22/03/25.
//

#include "ParticleSystem.hpp"

#include "spdlog/spdlog-inl.h"


ParticleSystem::ParticleSystem(Context &aContext, ParticleSystemSettings const &aSettings) : mSettings(mSettings)
{
    // get the number of particles from the settings
    int maxParticles = mSettings.maxParticles;
    // allocate the particles array
    mParticles = new Particle[maxParticles];
    // TODO: change the buffer allocation bits to be device only, we want to be updating this only in compute shaders
    mParticlesBuffer = CreateBuffer("particle system buffer", aContext, sizeof(Particle) * maxParticles,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                    VMA_ALLOCATION_CREATE_MAPPED_BIT);
    // TODO: create the descriptor set
    // TODO: create the pipeline

    // compute the time step which we release particles
    mTimeStepToEmit = 1.0 / aSettings.emission.rateOverTime;
    mDistanceStepToEmit = 1.0 / aSettings.emission.rateOverTime;
    // square the distance step
    mDistanceStepToEmit = mDistanceStepToEmit * mDistanceStepToEmit;
}

ParticleSystem::~ParticleSystem()
{
    // delete the particles array
    delete[] mParticles;
}

void ParticleSystem::Emit()
{
    // generate the position
    Emission emission;
    switch (mSettings.emissionShape.type)
    {
        case ShapeType::Sphere:
            emission = SphereParticleSpawn(seed++, mSettings.emissionShape.shape);
            break;
        case ShapeType::Cone:
            emission = ConeParticleSpawn(seed++, mSettings.emissionShape.shape);
            break;
        case ShapeType::Box:
            emission = BoxParticleSpawn(seed++, mSettings.emissionShape.shape);
        default:
            assert(false);
    }
    // generate the transform of the emitted shape from the location and velocity
    Transform emissionTransform = GenerateTransform(emission);
    // assign the emission to the back of the particle buffer
    mParticles[backOfParticleBuffer] = Particle{
        .transform = emissionTransform,
        .velocity = emission.Velocity * mSettings.startSpeed,
        .lifetime = mSettings.startLifetime,
        .colour = mSettings.startColour,
    };
    // increment the back of particle buffer pointer
    backOfParticleBuffer++;
    backOfParticleBuffer = backOfParticleBuffer % mSettings.maxParticles;
}

void ParticleSystem::UpdateParticles(double aDeltaTime)
{
    // lets work out the gravity addition
    glm::vec3 gravityAddition = static_cast<float>(aDeltaTime) * mSettings.gravityModifier;
    // for each particle, update
    for (auto &particle : mParticles)
    {
        // subtract deltatime from the lifetime
        particle.lifetime -= aDeltaTime;
        // we only need to update the rest if its still alive
        if (particle.lifetime > 0)
        {
            // first, lets add the gravity acceleration to the velocity
            particle.velocity = particle.velocity + gravityAddition;
            // next, add the movement from velocity to the transform
            particle.transform.position += particle.velocity * aDeltaTime;
            // finally, update the transform matrix
            particle.transform.UpdateMatrix();
        }
    }
}

void ParticleSystem::Update(double aDeltaTime, glm::vec3 aNewPosition)
{
    // first, lets multiply deltaTime by the simulation speed
    aDeltaTime *= mSettings.simulationSpeed;
    // now, lets work out how many particles we need to spawn
    mPreviousTime = mTime;
    mTime += aDeltaTime;
    size_t numberOfParticlesToEmit = 0;
    // round mPreviousTime up to the next multiple of mTimeStepToEmit, and count the number of time steps we pass
    for (double roundedUpTime = ceil(mPreviousTime / mTimeStepToEmit) * mTimeStepToEmit; roundedUpTime < mTime; roundedUpTime += mTimeStepToEmit)
    { numberOfParticlesToEmit++;}
    // calculate the new distance moved
    mPreviousPosition = mPosition;
    mPosition = aNewPosition;
    mPreviousDistance = mDistanceMoved;
    mDistanceMoved += glm::distance2(mPreviousPosition, mPosition);
    // round previous distance up to the next multiple of mDistanceStep, and count the number of distance steps we pass
    for (double roundedUpDistance = ceil(mPreviousDistance / mDistanceStepToEmit) * mDistanceStepToEmit; roundedUpDistance < mDistanceMoved; roundedUpDistance += mDistanceStepToEmit)
    { numberOfParticlesToEmit++;}
    // emit that number of particles
    for (size_t i = 0; i < numberOfParticlesToEmit; i++)
        Emit();

    // now we are done with emitting, it is time to update the particle positions
    UpdateParticles(aDeltaTime);
}


Emission SphereParticleSpawn(size_t seed, Shape const &sphereShape)
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
            } else
            {
                returnValue.Velocity = glm::normalize(spawnPosition);
            }
            spawnPosition *= sphereShape.sphere.radius;
            returnValue.Position = spawnPosition;
            return returnValue;
        }
    } while (true);
    assert(false);
}

Emission BoxParticleSpawn(size_t seed, Shape const &boxShape)
{
    assert(boxShape.box.emitFrom == EmitFrom::Volume);
    Emission returnValue{};
    // randomly generate a spawn position
    returnValue.Position = {
        get_random(seed++) * boxShape.box.size.x, get_random(seed++) * boxShape.box.size.y,
        get_random(seed++) * boxShape.box.size.z
    };
    returnValue.Velocity = glm::normalize(returnValue.Position);
    return returnValue;
}

Emission ConeParticleSpawn(size_t seed, Shape const &coneShape)
{
    // get the radius of the cone at the top - the bottom radius + the height * tan(angle)
    float tanTheta = glm::tan(coneShape.cone.angle);
    float topRadius = coneShape.cone.radius + coneShape.cone.height * tanTheta;
    Emission returnValue{};
    do
    {
        // randomly generate a spawn position, x and z are between -radius and radius, y is between 0 and height
        glm::vec3 spawnPosition = {
            get_random(seed++) * topRadius, get_hash(seed++) * coneShape.cone.height, get_random(seed++) * topRadius
        };
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
            } else
            {
                returnValue.Velocity = glm::normalize(spawnPosition);
            }
            returnValue.Position = spawnPosition;
            return returnValue;
        }
    } while (true);
}

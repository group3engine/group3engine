//
// Created by thomas on 22/03/25.
//

#include "ParticleSystem.hpp"

#include "Utils.hpp"

std::vector<ParticleSystem*> ParticleSystem::systems {};



ParticleSystem::ParticleSystem(Context &aContext, ParticleSystemSettings const &aSettings) : mSettings(aSettings), mContext(aContext)
{
    systems.push_back(this);
    // get the number of particles from the settings
    int maxParticles = mSettings.maxParticles;
    // allocate the particles array
    mParticles = new Particle[maxParticles] {};
    mParticleGPU = new ParticleGPU[maxParticles] {};
    mParticlesBuffer = CreateBuffer("particle system buffer", aContext, sizeof(ParticleGPU) * maxParticles,
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                    VMA_ALLOCATION_CREATE_MAPPED_BIT);
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    mDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(mContext, bindings);
    vkutil::AllocateDescriptorSet(mContext, mContext.descriptorPool, mDescriptorSetLayout, 1,  mDescriptorSet);
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = mParticlesBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ParticleGPU) * maxParticles;
    vkutil::UpdateDescriptorSet(mContext, 0, bufferInfo, mDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);


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
    // delete the particle gpu array
    delete[] mParticleGPU;
    // destroy the buffer
    mParticlesBuffer.Destroy();
    // destroy the descriptor set layout
    vkDestroyDescriptorSetLayout(mContext.device, mDescriptorSetLayout, nullptr);
    // remove this from the systems list
    systems.erase(std::remove(systems.begin(), systems.end(), this), systems.end());
}

void ParticleSystem::Emit()
{
    // generate the position
    Emission emission{};
    switch (mSettings.emissionShape.type)
    {
        case ShapeType::Sphere:
            emission = SphereParticleSpawn(seed, mSettings.emissionShape.shape);
            break;
        case ShapeType::Cone:
            emission = ConeParticleSpawn(seed, mSettings.emissionShape.shape);
            break;
        case ShapeType::Box:
            emission = BoxParticleSpawn(seed, mSettings.emissionShape.shape);
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

Transform ParticleSystem::GenerateTransform(Emission const &aEmission) const
{
    // ok so we need to generate this transform using the following properties:
    // the Emission.position, the Emission.velocity, the startSize, the startRotation, emissionshape.alignToDirection
    Transform resultingTransform {};
    // first off, the translation of the resulting transform is just the emission.position
    resultingTransform.translation = aEmission.Position + mSettings.attachedEntity->GetWorldTransformComponents().translation;
    // the scale of the transform is just the startSize
    resultingTransform.scale = mSettings.startSize;
    // the rotation is the alignToDirection rotation * the startRotation
    // first, lets calculate the align to direction rotation if we need to
    resultingTransform.rotation = glm::quat(0,0,0,1);
    if (mSettings.emissionShape.alignToDirection)
    {
        resultingTransform.rotation = glm::lookAt({0.0f, 0.0f, 0.0f},aEmission.Velocity, {0.0f, 1.0f, 0.0f});
    }
    resultingTransform.rotation = mSettings.startRotation * resultingTransform.rotation;
    resultingTransform.rotation = glm::normalize(resultingTransform.rotation);
    resultingTransform.UpdateMatrix();
    return resultingTransform;

}

void ParticleSystem::UpdateParticles(double aDeltaTime)
{
    // lets work out the gravity addition
    glm::vec3 gravityAddition = static_cast<float>(aDeltaTime) * mSettings.gravityModifier;
    // for each particle, update
    for (size_t i=0; i < mSettings.maxParticles; i++)
    {
        Particle &particle = mParticles[i];
        ParticleGPU &gpu = mParticleGPU[i];
        // subtract deltatime from the lifetime
        particle.lifetime -= aDeltaTime;
        // we only need to update the rest if its still alive
        if (particle.lifetime > 0)
        {
            // first, lets add the gravity acceleration to the velocity
            particle.velocity = particle.velocity + gravityAddition;
            // next, add the movement from velocity to the transform
            particle.transform.translation += particle.velocity * static_cast<float>(aDeltaTime);
            // finally, update the transform matrix
            particle.transform.UpdateMatrix();
            // update the gpu particle
            gpu.transform = particle.transform.getMatrix();
            gpu.colour = particle.colour;
            gpu.isEnabled = true;
        }
        else
        {
            gpu.isEnabled = false;
        }

    }

    mParticlesBuffer.Update(mContext, mParticleGPU, mSettings.maxParticles * sizeof(ParticleGPU));
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

void ParticleSystem::DrawAll(VkCommandBuffer cmd, VkPipelineLayout aLayout)
{
    for (ParticleSystem* sys : systems)
    {
        sys->Render(cmd, aLayout);
    }
}

void ParticleSystem::Render(VkCommandBuffer cmd, VkPipelineLayout aLayout)
{
    // bind the particle descriptor
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, aLayout, 2, 1, &mDescriptorSet, 0, nullptr);
    // bind the material
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, aLayout, 1,
                            1, &mSettings.material->descriptorSet, 0, nullptr);

    // bind the vertex buffers - positions, texcoords, normals
    VkBuffer buffers[] = {mSettings.renderSettings.mesh->meshPrimitives[0].meshGPU->mVertices.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, sizeof(buffers) / sizeof(buffers[0]), buffers,
                           offsets);

    // bind the index buffer
    vkCmdBindIndexBuffer(cmd, mSettings.renderSettings.mesh->meshPrimitives[0].meshGPU->mIndices.buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    // draw the mesh
    vkCmdDrawIndexed(cmd, mSettings.renderSettings.mesh->meshPrimitives[0].meshGPU->mIndexCount, mSettings.maxParticles, 0, 0, 0);
}





Emission SphereParticleSpawn(size_t& seed, EmitterShape const &sphereShape)
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

Emission BoxParticleSpawn(size_t &seed, EmitterShape const &boxShape)
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

Emission ConeParticleSpawn(size_t &seed, EmitterShape const &coneShape)
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

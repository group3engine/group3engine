//
// Created by thomas on 27/03/25.
//

#include "ParticleCube.hpp"

ParticleCube::ParticleCube() {

}

void ParticleCube::Update(double deltaTime)
{
    mParticleSystem->Update(deltaTime);
}

void ParticleCube::Awake()
{
    // create the particle system
    ParticleSystemSettings pSettings {};
    pSettings.renderSettings.mesh = GetMesh();
    pSettings.material = GetMesh()->meshPrimitives[0].material;
    pSettings.emissionShape.shape.cone.angle = 1.f;
    pSettings.emissionShape.shape.cone.radius = 1.f;
    pSettings.emissionShape.shape.cone.height = 1.f;
    pSettings.emissionShape.alignToDirection = true;
    pSettings.attachedEntity = this;
    pSettings.emission.rateOverTime = 20.f;
    mParticleSystem = new ParticleSystem(Context::get(), pSettings);
}

ParticleCube::~ParticleCube()
{
    delete mParticleSystem;
}


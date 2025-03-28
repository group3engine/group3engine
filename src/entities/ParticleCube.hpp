//
// Created by thomas on 27/03/25.
//

#ifndef GROUP3ENGINE_PARTICLECUBE_HPP
#define GROUP3ENGINE_PARTICLECUBE_HPP
#include "Entity.hpp"
#include "ParticleSystem.hpp"


class ParticleCube : public Entity {

public:
    ParticleCube();
    ~ParticleCube();

    void Awake() override;
    void Update(double deltaTime) override;

private:
    ParticleSystem *mParticleSystem;
};


#endif //GROUP3ENGINE_PARTICLECUBE_HPP

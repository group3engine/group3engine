//
// Created by thomas on 20/03/25.
//

#ifndef SAMPLENPCENTITY_HPP
#define SAMPLENPCENTITY_HPP


#include "CharacterEntity.hpp"
#include "Entity.hpp"
#include "SampleJoltCharacter.h"



class SampleSecondaryEntity : public CharacterEntity {
  public:
    SampleSecondaryEntity();
    ~SampleSecondaryEntity() override;

    void Awake() override;

    void PreUpdate(double deltaTime) override;

    void ProcessInput() override;
};

#endif //SAMPLENPCENTITY_HPP

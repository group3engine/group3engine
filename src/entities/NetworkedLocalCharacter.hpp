//
// Created by thomas on 03/04/25.
//

#ifndef GROUP3ENGINE_NETWORKEDLOCALCHARACTER_HPP
#define GROUP3ENGINE_NETWORKEDLOCALCHARACTER_HPP
#include "CharacterEntity.hpp"

class NetworkedLocalCharacter : public CharacterEntity {

public:
    NetworkedLocalCharacter();
    ~NetworkedLocalCharacter() override = default;

    void Update(double deltaTime) override;
};


#endif //GROUP3ENGINE_NETWORKEDLOCALCHARACTER_HPP

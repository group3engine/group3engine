//
// Created by thomas on 03/05/25.
//

#ifndef GROUP3ENGINE_ANIMATIONTEST_HPP
#define GROUP3ENGINE_ANIMATIONTEST_HPP
#include "Entity.hpp"

class AnimationTest : public Entity{
public:
    AnimationTest() = default;
    ~AnimationTest() override = default;

    void Awake() override;
private:
    std::string animationName = "dance";
};


#endif //GROUP3ENGINE_ANIMATIONTEST_HPP

//
// Created by thomas on 03/05/25.
//

#include "AnimationTest.hpp"

void AnimationTest::Awake()
{
    // set the animation to play
    // for each child, if there is an animator, call set animation
    for (auto *child : GetChildren()) {
        if (child->HasAnimator()) {
            child->GetAnimator().SetActiveAnimation(animationName, 0, false, true);
            break;
        }
    }
}

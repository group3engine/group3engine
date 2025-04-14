//
// Created by thomas on 07/04/25.
//

#include "TileBox.hpp"

void TileBox::OnCollisionStart(Entity *aOther)
{
    // check if the other entity is a character
    if (aOther->CompareTag("character") && !pressed)
    {
        // set the animation of the parent
        GetParent()->GetAnimator().SetActiveAnimation("tiledown", 0.f, false, false);
        pressed = true;
    }

}

void TileBox::Update(double aDeltaTime)
{
    if(pressed)
    {
        GetParent()->GetAnimator().SetActiveAnimation("tilestaydown", 0.2f, false, true);
    }
    else
    {
        GetParent()->GetAnimator().SetActiveAnimation("tilestayup", 0.2f, false);
    }
}

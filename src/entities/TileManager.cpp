//
// Created by thomas on 07/04/25.
//

#include "TileManager.hpp"

void TileManager::Awake()
{
    // go through the children and get the tiles - the first 12 children are tiles
    auto children = GetChildren();
    for (size_t i = 0; i < 12; ++i)
    {
        // this is informed by the heirarchy set by the artist
        auto *child = static_cast<TileBox*>(children[i]->GetChildren()[1]->GetChildren()[0]);
        // make sure the childs type is a tilebox
        if (child && child->CompareType("tileBox"))
        {
            mTileBoxes.push_back(child);
        }
    }
    // the monkey is the last child
    mMonkey = children.back()->GetChildren()[1];
    mMonkey->GetAnimator().SetActiveAnimation("monkeyidle");

}

void TileManager::Update(double aDeltaTime)
{
    // if all the tiles are pressed, set the monkey to spin
    bool allPressed = true;
    for (auto *tile : mTileBoxes)
    {
        if (!tile->IsPressed())
        {
            allPressed = false;
            break;
        }
    }
    if(allPressed)
    {
        mMonkey->GetAnimator().SetActiveAnimation("monkeyspin", 0.f, false, true);
    }

}

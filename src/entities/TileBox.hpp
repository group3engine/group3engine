//
// Created by thomas on 07/04/25.
//

#ifndef GROUP3ENGINE_TILEBOX_HPP
#define GROUP3ENGINE_TILEBOX_HPP
#include "Entity.hpp"

class TileBox : public Entity{

public:
    TileBox() {mType = "tileBox";}
    void OnCollisionStart(Entity *aOther) override;
    void UnPress(){
        pressed = false;
        GetParent()->GetAnimator().SetActiveAnimation("tileup", 0.f, false, false);
    }
    bool IsPressed() const { return pressed; }

    void Update(double aDeltaTime) override;

private:
    bool pressed = false;

};


#endif //GROUP3ENGINE_TILEBOX_HPP

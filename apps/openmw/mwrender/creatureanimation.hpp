#ifndef _GAME_RENDER_CREATUREANIMATION_H
#define _GAME_RENDER_CREATUREANIMATION_H

#include "animation.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    class CreatureAnimation : public Animation
    {
    public:
        CreatureAnimation(const MWWorld::Ptr& ptr);
        virtual ~CreatureAnimation();
    };
}

#endif

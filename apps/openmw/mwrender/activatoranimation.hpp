#ifndef GAME_RENDER_ACTIVATORANIMATION_H
#define GAME_RENDER_ACTIVATORANIMATION_H

#include "animation.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    class ActivatorAnimation : public Animation
    {
    public:
        ActivatorAnimation(const MWWorld::Ptr& ptr);
        virtual ~ActivatorAnimation();
    };
}

#endif

#ifndef _GAME_RENDER_CREATUREANIMATION_H
#define _GAME_RENDER_CREATUREANIMATION_H

#include "animation.hpp"

#include "components/nifogre/ogre_nif_loader.hpp"


namespace MWRender{

class CreatureAnimation: public Animation{

    public:
    virtual ~CreatureAnimation();
    CreatureAnimation(const MWWorld::Ptr& ptr, OEngine::Render::OgreRenderer& _rend);
    virtual void runAnimation(float timepassed);

};
}
#endif

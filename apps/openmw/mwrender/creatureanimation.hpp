#ifndef _GAME_RENDER_CREATUREANIMATION_H
#define _GAME_RENDER_CREATUREANIMATION_H

#include "animation.hpp"
#include <components/nif/node.hpp>


#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/environment.hpp"
#include "components/nifogre/ogre_nif_loader.hpp"


namespace MWRender{

class CreatureAnimation: public Animation{
   
    public:
    ~CreatureAnimation();
    CreatureAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env, OEngine::Render::OgreRenderer& _rend);
	virtual void runAnimation(float timepassed);
	

};
}
#endif
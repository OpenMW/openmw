#ifndef _GAME_RENDER_NPCANIMATION_H
#define _GAME_RENDER_NPCANIMATION_H
#include "animation.hpp"
#include <components/nif/data.hpp>
#include <components/nif/node.hpp>
#include <components/nif/property.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/extra.hpp>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/environment.hpp"
#include "components/nifogre/ogre_nif_loader.hpp"

namespace MWRender{

class NpcAnimation: public Animation{
    
    
    
    public:
     NpcAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env, OEngine::Render::OgreRenderer& _rend);
     ~NpcAnimation();
    Ogre::Entity* insertBoundedPart(const std::string &mesh, std::string bonename);
    void insertFreePart(const std::string &mesh, const std::string suffix, Ogre::SceneNode* insert);
	virtual void runAnimation(float timepassed);
	
};
}
#endif
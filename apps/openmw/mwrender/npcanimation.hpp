#ifndef _GAME_RENDER_NPCANIMATION_H
#define _GAME_RENDER_NPCANIMATION_H
#include "animation.hpp"
#include <components/nif/data.hpp>
#include <components/nif/node.hpp>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/environment.hpp"
#include "components/nifogre/ogre_nif_loader.hpp"

namespace MWRender{

class NpcAnimation: public Animation{
    std::vector<std::vector<Nif::NiTriShapeCopy>> shapeparts;   //All the NiTriShape data that we need for animating this particular npc
    public:
     NpcAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env, OEngine::Render::OgreRenderer& _rend);
     ~NpcAnimation();
    Ogre::Entity* insertBoundedPart(const std::string &mesh, std::string bonename);
};
}
#endif
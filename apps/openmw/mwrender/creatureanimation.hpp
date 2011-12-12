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
    std::vector<Nif::NiTriShapeCopy> shapes;          //All the NiTriShapeData for this creature
    public:
    CreatureAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env, OEngine::Render::OgreRenderer& _rend);
};
}
#endif
#include "creatureanimation.hpp"

#include "../mwworld/world.hpp"

using namespace Ogre;
using namespace NifOgre;
namespace MWRender{
CreatureAnimation::CreatureAnimation(const MWWorld::Ptr& ptr, MWWorld::Environment& _env,OEngine::Render::OgreRenderer& _rend): Animation(_env,_rend){
    Ogre::SceneNode* insert = ptr.getRefData().getBaseNode();
    assert(insert);
    ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref =
            ptr.get<ESM::Creature>();

   assert (ref->base != NULL);
    if(!ref->base->model.empty()){
        const std::string &mesh = "meshes\\" + ref->base->model;

        NifOgre::NIFLoader::load(mesh);
        base = mRend.getScene()->createEntity(mesh);
        insert->attachObject(base);
    }
}
}
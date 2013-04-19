#include "creatureanimation.hpp"

#include "renderconst.hpp"

#include "../mwbase/world.hpp"

namespace MWRender
{

CreatureAnimation::~CreatureAnimation()
{
}

CreatureAnimation::CreatureAnimation(const MWWorld::Ptr &ptr)
  : Animation(ptr)
{
    MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();

    assert (ref->mBase != NULL);
    if(!ref->mBase->mModel.empty())
    {
        std::string model = "meshes\\"+ref->mBase->mModel;

        if((ref->mBase->mFlags&ESM::Creature::Biped))
            addObjectList(mPtr.getRefData().getBaseNode(), "meshes\\base_anim.nif", true);

        addObjectList(mPtr.getRefData().getBaseNode(), model, false);
        setRenderProperties(mObjectLists.back(), RV_Actors, RQG_Main, RQG_Alpha);
    }
}

}

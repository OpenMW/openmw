#include "activatoranimation.hpp"

#include "renderconst.hpp"

#include "../mwbase/world.hpp"

namespace MWRender
{

ActivatorAnimation::~ActivatorAnimation()
{
}

ActivatorAnimation::ActivatorAnimation(const MWWorld::Ptr &ptr)
  : Animation(ptr)
{
    MWWorld::LiveCellRef<ESM::Activator> *ref = mPtr.get<ESM::Activator>();

    assert (ref->mBase != NULL);
    if(!ref->mBase->mModel.empty())
    {
        const std::string name = "meshes\\"+ref->mBase->mModel;

        addObjectList(mPtr.getRefData().getBaseNode(), name, false);
        setRenderProperties(mObjectLists.back(), RV_Misc, RQG_Main, RQG_Alpha);
    }
}

}

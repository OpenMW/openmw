#include "activatoranimation.hpp"

#include <components/esm/loadacti.hpp>

#include "../mwbase/world.hpp"

#include "renderconst.hpp"

namespace MWRender
{

ActivatorAnimation::~ActivatorAnimation()
{
}

ActivatorAnimation::ActivatorAnimation(const MWWorld::Ptr &ptr)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    MWWorld::LiveCellRef<ESM::Activator> *ref = mPtr.get<ESM::Activator>();

    assert(ref->mBase != NULL);
    if(!ref->mBase->mModel.empty())
    {
        const std::string name = "meshes\\"+ref->mBase->mModel;

        setObjectRoot(name, false);
        setRenderProperties(mObjectRoot, RV_Misc, RQG_Main, RQG_Alpha);

        addAnimSource(name);
    }
}

}

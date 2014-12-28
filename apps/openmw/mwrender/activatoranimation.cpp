#include "activatoranimation.hpp"

#include <components/esm/loadacti.hpp>

#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "renderconst.hpp"

namespace MWRender
{

ActivatorAnimation::~ActivatorAnimation()
{
}

ActivatorAnimation::ActivatorAnimation(const MWWorld::Ptr &ptr)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    const std::string& model = mPtr.getClass().getModel(mPtr);

    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Misc, RQG_Main, RQG_Alpha);

        addAnimSource(model);
    }
}

}

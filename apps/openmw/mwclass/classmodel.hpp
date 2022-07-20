#ifndef OPENMW_MWCLASS_CLASSMODEL_H
#define OPENMW_MWCLASS_CLASSMODEL_H

#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/livecellref.hpp"

#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <string>

namespace MWClass
{
    template <class Class>
    std::string getClassModel(const MWWorld::ConstPtr& ptr)
    {
        const MWWorld::LiveCellRef<Class> *ref = ptr.get<Class>();

        if (!ref->mBase->mModel.empty())
            return Misc::ResourceHelpers::correctMeshPath(ref->mBase->mModel,
                MWBase::Environment::get().getResourceSystem()->getVFS());

        return {};
    }
}

#endif

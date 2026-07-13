#ifndef OPENMW_MWCLASS_CLASSMODEL_H
#define OPENMW_MWCLASS_CLASSMODEL_H

#include "../mwworld/livecellref.hpp"
#include "../mwworld/ptr.hpp"

#include <components/esm/path.hpp>
#include <components/vfs/pathutil.hpp>

namespace MWClass
{
    template <class Class>
    VFS::Path::NormalizedView getClassModel(const MWWorld::ConstPtr& ptr)
    {
        const MWWorld::LiveCellRef<Class>* ref = ptr.get<Class>();
        return ref->mBase->mModel.getNormalized();
    }
}

#endif

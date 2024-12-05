#ifndef OPENMW_MWCLASS_CLASSMODEL_H
#define OPENMW_MWCLASS_CLASSMODEL_H

#include "../mwworld/livecellref.hpp"
#include "../mwworld/ptr.hpp"

#include <string_view>

namespace MWClass
{
    template <class Class>
    std::string_view getClassModel(const MWWorld::ConstPtr& ptr)
    {
        const MWWorld::LiveCellRef<Class>* ref = ptr.get<Class>();
        return ref->mBase->mModel;
    }
}

#endif

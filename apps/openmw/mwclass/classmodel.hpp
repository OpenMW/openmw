#ifndef OPENMW_MWCLASS_CLASSMODEL_H
#define OPENMW_MWCLASS_CLASSMODEL_H

#include "../mwworld/livecellref.hpp"
#include "../mwworld/ptr.hpp"

#include <components/esm/path.hpp>

#include <string_view>
#include <type_traits>

namespace MWClass
{
    template <class Class>
    std::string_view getClassModel(const MWWorld::ConstPtr& ptr)
    {
        const MWWorld::LiveCellRef<Class>* ref = ptr.get<Class>();
        if constexpr (std::is_same_v<decltype(ref->mBase->mModel), ESM::Path>)
            return ref->mBase->mModel.getOriginal();
        else
            return ref->mBase->mModel;
    }
}

#endif

#ifndef OPENMW_MWCLASS_NAMEORID_H
#define OPENMW_MWCLASS_NAMEORID_H

#include <components/esm/refid.hpp>

#include "../mwworld/livecellref.hpp"
#include "../mwworld/ptr.hpp"

#include <string_view>

namespace MWClass
{
    template <class Class>
    std::string_view getNameOrId(const MWWorld::ConstPtr& ptr)
    {
        const MWWorld::LiveCellRef<Class>* ref = ptr.get<Class>();
        if (!ref->mBase->mName.empty())
            return ref->mBase->mName;
        if (const auto* id = ref->mBase->mId.template getIf<ESM::StringRefId>())
            return id->getValue();
        return {};
    }
}

#endif

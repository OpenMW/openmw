#include "object.hpp"

#include "types/types.hpp"

#include <components/misc/resourcehelpers.hpp>

namespace MWLua
{

    std::string idToString(const ObjectId& id)
    {
        return std::to_string(id.mIndex) + "_" + std::to_string(id.mContentFile);
    }

    bool isMarker(const MWWorld::Ptr& ptr)
    {
        return Misc::ResourceHelpers::isHiddenMarker(ptr.getCellRef().getRefId());
    }

    std::string ptrToString(const MWWorld::Ptr& ptr)
    {
        std::string res = "object";
        res.append(idToString(getId(ptr)));
        res.append(" (");
        res.append(getLuaObjectTypeName(ptr));
        res.append(", ");
        res.append(ptr.getCellRef().getRefId().getRefIdString());
        res.append(")");
        return res;
    }

    std::string Object::toString() const
    {
        if (isValid())
            return ptrToString(ptr());
        else
            return "object" + idToString(mId) + " (not found)";
    }

    bool Object::isValid() const
    {
        MWWorld::WorldModel& w = *MWBase::Environment::get().getWorldModel();
        if (mLastUpdate < w.getPtrIndexUpdateCounter())
        {
            mPtr = w.getPtr(mId);
            mLastUpdate = w.getPtrIndexUpdateCounter();
        }
        return !mPtr.isEmpty();
    }

    const MWWorld::Ptr& Object::ptr() const
    {
        if (!isValid())
            throw std::runtime_error("Object is not available: " + idToString(mId));
        return mPtr;
    }
}

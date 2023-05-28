#include "ptr.hpp"

#include "apps/openmw/mwbase/environment.hpp"

#include "worldmodel.hpp"

namespace MWWorld
{

    std::string Ptr::toString() const
    {
        std::string res = "object";
        if (getRefData().isDeleted())
            res = "deleted object";
        res.append(getCellRef().getRefNum().toString());
        res.append(" (");
        res.append(getTypeDescription());
        res.append(", ");
        res.append(getCellRef().getRefId().toDebugString());
        res.append(")");
        return res;
    }

    SafePtr::SafePtr(const Ptr& ptr)
        : mId(ptr.getCellRef().getRefNum())
        , mPtr(ptr)
        , mLastUpdate(MWBase::Environment::get().getWorldModel()->getPtrRegistry().getRevision())
    {
    }

    std::string SafePtr::toString() const
    {
        update();
        if (mPtr.isEmpty())
            return "object" + mId.toString() + " (not found)";
        else
            return mPtr.toString();
    }

    void SafePtr::update() const
    {
        const PtrRegistry& registry = MWBase::Environment::get().getWorldModel()->getPtrRegistry();
        if (mLastUpdate != registry.getRevision())
        {
            mPtr = registry.getOrDefault(mId);
            mLastUpdate = registry.getRevision();
        }
    }
}

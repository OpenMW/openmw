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
        res.append(getCellRef().getRefId().getRefIdString());
        res.append(")");
        return res;
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
        WorldModel& w = *MWBase::Environment::get().getWorldModel();
        if (mLastUpdate < w.getPtrIndexUpdateCounter())
        {
            mPtr = w.getPtr(mId);
            mLastUpdate = w.getPtrIndexUpdateCounter();
        }
    }
}

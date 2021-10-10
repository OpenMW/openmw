#include "object.hpp"

#include "../mwclass/activator.hpp"
#include "../mwclass/armor.hpp"
#include "../mwclass/book.hpp"
#include "../mwclass/clothing.hpp"
#include "../mwclass/container.hpp"
#include "../mwclass/creature.hpp"
#include "../mwclass/door.hpp"
#include "../mwclass/ingredient.hpp"
#include "../mwclass/light.hpp"
#include "../mwclass/misc.hpp"
#include "../mwclass/npc.hpp"
#include "../mwclass/potion.hpp"
#include "../mwclass/static.hpp"
#include "../mwclass/weapon.hpp"

namespace MWLua
{

    std::string idToString(const ObjectId& id)
    {
        return std::to_string(id.mIndex) + "_" + std::to_string(id.mContentFile);
    }

    bool isMarker(const MWWorld::Ptr& ptr)
    {
        std::string_view id = ptr.getCellRef().getRefIdRef();
        return id == "prisonmarker" || id == "divinemarker" || id == "templemarker" || id == "northmarker";
    }

    std::string getMWClassName(const MWWorld::Ptr& ptr)
    {
        if (ptr.getCellRef().getRefIdRef() == "player")
            return "Player";
        if (isMarker(ptr))
            return "Marker";
        return ptr.getTypeName();
    }

    std::string ptrToString(const MWWorld::Ptr& ptr)
    {
        std::string res = "object";
        res.append(idToString(getId(ptr)));
        res.append(" (");
        res.append(getMWClassName(ptr));
        res.append(", ");
        res.append(ptr.getCellRef().getRefIdRef());
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
        if (mLastUpdate < mObjectRegistry->mUpdateCounter)
        {
            updatePtr();
            mLastUpdate = mObjectRegistry->mUpdateCounter;
        }
        return !mPtr.isEmpty();
    }

    const MWWorld::Ptr& Object::ptr() const
    {
        if (!isValid())
            throw std::runtime_error("Object is not available: " + idToString(mId));
        return mPtr;
    }

    void ObjectRegistry::update()
    {
        if (mChanged)
        {
            mUpdateCounter++;
            mChanged = false;
        }
    }

    void ObjectRegistry::clear()
    {
        mObjectMapping.clear();
        mChanged = false;
        mUpdateCounter = 0;
        mLastAssignedId.unset();
    }

    MWWorld::Ptr ObjectRegistry::getPtr(ObjectId id, bool local)
    {
        MWWorld::Ptr ptr;
        auto it = mObjectMapping.find(id);
        if (it != mObjectMapping.end())
            ptr = it->second;
        if (local)
        {
            // TODO: Return ptr only if it is active or was active in the previous frame, otherwise return empty.
            //     Needed because in multiplayer inactive objects will not be synchronized, so an be out of date.
        }
        else
        {
            // TODO: If Ptr is empty then try to load the object from esp/esm.
        }
        return ptr;
    }

    ObjectId ObjectRegistry::registerPtr(const MWWorld::Ptr& ptr)
    {
        ObjectId id = ptr.getCellRef().getOrAssignRefNum(mLastAssignedId);
        mChanged = true;
        mObjectMapping[id] = ptr;
        return id;
    }

    ObjectId ObjectRegistry::deregisterPtr(const MWWorld::Ptr& ptr)
    {
        ObjectId id = getId(ptr);
        mChanged = true;
        mObjectMapping.erase(id);
        return id;
    }

}

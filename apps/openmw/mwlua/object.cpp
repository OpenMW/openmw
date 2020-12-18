#include "object.hpp"

#include <components/esm/loadnpc.hpp>
#include <components/esm/loadcrea.hpp>

namespace MWLua
{

    std::string idToString(const ObjectId& id)
    {
        return std::to_string(id.mIndex) + "_" + std::to_string(id.mContentFile);
    }

    std::string Object::toString() const
    {
        std::string res = idToString(mId);
        if (isValid())
        {
            res.append(" (");
            res.append(type());
            res.append(", ");
            res.append(*ptr().getCellRef().getRefIdPtr());
            res.append(")");
        }
        else
            res.append(" (not found)");
        return res;
    }

    std::string_view Object::type() const
    {
        if (*ptr().getCellRef().getRefIdPtr() == "player")
            return "Player";
        const std::string& typeName = ptr().getTypeName();
        if (typeName == typeid(ESM::NPC).name())
            return "NPC";
        else if (typeName == typeid(ESM::Creature).name())
            return "Creature";
        else
            return typeName;
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

    MWWorld::Ptr ObjectRegistry::getPtr(ObjectId id, bool onlyActive)
    {
        MWWorld::Ptr ptr;
        auto it = mObjectMapping.find(id);
        if (it != mObjectMapping.end())
            ptr = it->second;
        if (onlyActive)
        {
            // TODO: add flag `isActive` to LiveCellRefBase. Return empty Ptr if the flag is not set.
            //     Needed because in multiplayer mode inactive objects will not be synchronized, so will likely be out of date.
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

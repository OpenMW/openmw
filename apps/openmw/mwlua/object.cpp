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

    const static std::map<std::type_index, std::string_view> classNames = {
        {typeid(MWClass::Activator), "Activator"},
        {typeid(MWClass::Armor), "Armor"},
        {typeid(MWClass::Book), "Book"},
        {typeid(MWClass::Clothing), "Clothing"},
        {typeid(MWClass::Container), "Container"},
        {typeid(MWClass::Creature), "Creature"},
        {typeid(MWClass::Door), "Door"},
        {typeid(MWClass::Ingredient), "Ingredient"},
        {typeid(MWClass::Light), "Light"},
        {typeid(MWClass::Miscellaneous), "Miscellaneous"},
        {typeid(MWClass::Npc), "NPC"},
        {typeid(MWClass::Potion), "Potion"},
        {typeid(MWClass::Static), "Static"},
        {typeid(MWClass::Weapon), "Weapon"},
    };

    std::string_view getMWClassName(const std::type_index& cls_type, std::string_view fallback)
    {
        auto it = classNames.find(cls_type);
        if (it != classNames.end())
            return it->second;
        else
            return fallback;
    }

    bool isMarker(const MWWorld::Ptr& ptr)
    {
        std::string_view id = *ptr.getCellRef().getRefIdPtr();
        return id == "prisonmarker" || id == "divinemarker" || id == "templemarker" || id == "northmarker";
    }

    std::string_view getMWClassName(const MWWorld::Ptr& ptr)
    {
        if (*ptr.getCellRef().getRefIdPtr() == "player")
            return "Player";
        if (isMarker(ptr))
            return "Marker";
        return getMWClassName(typeid(ptr.getClass()), ptr.getTypeName());
    }

    std::string ptrToString(const MWWorld::Ptr& ptr)
    {
        std::string res = "object";
        res.append(idToString(getId(ptr)));
        res.append(" (");
        res.append(getMWClassName(ptr));
        res.append(", ");
        res.append(*ptr.getCellRef().getRefIdPtr());
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

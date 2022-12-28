#ifndef MWLUA_OBJECT_H
#define MWLUA_OBJECT_H

#include <map>
#include <typeindex>

#include <sol/sol.hpp>

#include <components/esm3/cellref.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/worldmodel.hpp"

namespace MWLua
{
    // ObjectId is a unique identifier of a game object.
    // It can change only if the order of content files was change.
    using ObjectId = ESM::RefNum;
    inline const ObjectId& getId(const MWWorld::Ptr& ptr)
    {
        return ptr.getCellRef().getRefNum();
    }
    std::string idToString(const ObjectId& id);
    std::string ptrToString(const MWWorld::Ptr& ptr);
    bool isMarker(const MWWorld::Ptr& ptr);

    // Lua scripts can't use MWWorld::Ptr directly, because lifetime of a script can be longer than lifetime of Ptr.
    // `GObject` and `LObject` are intended to be passed to Lua as a userdata.
    // It automatically updates the underlying Ptr when needed.
    class Object
    {
    public:
        Object(ObjectId id)
            : mId(id)
        {
        }
        virtual ~Object() {}
        ObjectId id() const { return mId; }

        std::string toString() const;

        // Updates and returns the underlying Ptr. Throws an exception if object is not available.
        const MWWorld::Ptr& ptr() const;

        // Returns `true` if calling `ptr()` is safe.
        bool isValid() const;

        virtual sol::object getObject(lua_State* lua, ObjectId id) const = 0; // returns LObject or GOBject
        virtual sol::object getCell(lua_State* lua, MWWorld::CellStore* store) const = 0; // returns LCell or GCell

    protected:
        const ObjectId mId;

        mutable MWWorld::Ptr mPtr;
        mutable size_t mLastUpdate = 0;
    };

    // Used only in local scripts
    struct LCell
    {
        MWWorld::CellStore* mStore;
    };
    class LObject : public Object
    {
        using Object::Object;
        sol::object getObject(lua_State* lua, ObjectId id) const final { return sol::make_object<LObject>(lua, id); }
        sol::object getCell(lua_State* lua, MWWorld::CellStore* store) const final
        {
            return sol::make_object(lua, LCell{ store });
        }
    };

    // Used only in global scripts
    struct GCell
    {
        MWWorld::CellStore* mStore;
    };
    class GObject : public Object
    {
        using Object::Object;
        sol::object getObject(lua_State* lua, ObjectId id) const final { return sol::make_object<GObject>(lua, id); }
        sol::object getCell(lua_State* lua, MWWorld::CellStore* store) const final
        {
            return sol::make_object(lua, GCell{ store });
        }
    };

    using ObjectIdList = std::shared_ptr<std::vector<ObjectId>>;
    template <typename Obj>
    struct ObjectList
    {
        ObjectIdList mIds;
    };
    using GObjectList = ObjectList<GObject>;
    using LObjectList = ObjectList<LObject>;

    template <typename Obj>
    struct Inventory
    {
        Obj mObj;
    };

}

#endif // MWLUA_OBJECT_H

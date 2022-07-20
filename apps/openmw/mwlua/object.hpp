#ifndef MWLUA_OBJECT_H
#define MWLUA_OBJECT_H

#include <typeindex>
#include <map>

#include <sol/sol.hpp>

#include <components/esm3/cellref.hpp>

#include "../mwworld/ptr.hpp"

namespace MWLua
{
    // ObjectId is a unique identifier of a game object.
    // It can change only if the order of content files was change.
    using ObjectId = ESM::RefNum;
    inline const ObjectId& getId(const MWWorld::Ptr& ptr) { return ptr.getCellRef().getRefNum(); }
    std::string idToString(const ObjectId& id);
    std::string ptrToString(const MWWorld::Ptr& ptr);
    bool isMarker(const MWWorld::Ptr& ptr);

    // Holds a mapping ObjectId -> MWWord::Ptr.
    class ObjectRegistry
    {
    public:
        ObjectRegistry() { mLastAssignedId.unset(); }

        void update();  // Should be called every frame.
        void clear();  // Should be called before starting or loading a new game.

        ObjectId registerPtr(const MWWorld::Ptr& ptr);
        ObjectId deregisterPtr(const MWWorld::Ptr& ptr);

        // Returns Ptr by id. If object is not found, returns empty Ptr.
        // If local = true, returns non-empty ptr only if it can be used in local scripts
        // (i.e. is active or was active in the previous frame).
        MWWorld::Ptr getPtr(ObjectId id, bool local);

        // Needed only for saving/loading.
        const ObjectId& getLastAssignedId() const { return mLastAssignedId; }
        void setLastAssignedId(ObjectId id) { mLastAssignedId = id; }

    private:
        friend class Object;
        friend class LuaManager;

        bool mChanged = false;
        int64_t mUpdateCounter = 0;
        std::map<ObjectId, MWWorld::Ptr> mObjectMapping;
        ObjectId mLastAssignedId;
    };

    // Lua scripts can't use MWWorld::Ptr directly, because lifetime of a script can be longer than lifetime of Ptr.
    // `GObject` and `LObject` are intended to be passed to Lua as a userdata.
    // It automatically updates the underlying Ptr when needed.
    class Object
    {
    public:
        Object(ObjectId id, ObjectRegistry* reg) : mId(id), mObjectRegistry(reg) {}
        virtual ~Object() {}
        ObjectId id() const { return mId; }

        std::string toString() const;

        // Updates and returns the underlying Ptr. Throws an exception if object is not available.
        const MWWorld::Ptr& ptr() const;

        // Returns `true` if calling `ptr()` is safe.
        bool isValid() const;

        virtual sol::object getObject(lua_State* lua, ObjectId id) const = 0;  // returns LObject or GOBject
        virtual sol::object getCell(lua_State* lua, MWWorld::CellStore* store) const = 0;  // returns LCell or GCell

    protected:
        virtual void updatePtr() const = 0;

        const ObjectId mId;
        ObjectRegistry* mObjectRegistry;

        mutable MWWorld::Ptr mPtr;
        mutable int64_t mLastUpdate = -1;
    };

    // Used only in local scripts
    struct LCell
    {
        MWWorld::CellStore* mStore;
    };
    class LObject : public Object
    {
        using Object::Object;
        void updatePtr() const final { mPtr = mObjectRegistry->getPtr(mId, true); }
        sol::object getObject(lua_State* lua, ObjectId id) const final { return sol::make_object<LObject>(lua, id, mObjectRegistry); }
        sol::object getCell(lua_State* lua, MWWorld::CellStore* store) const final { return sol::make_object(lua, LCell{store}); }
    };

    // Used only in global scripts
    struct GCell
    {
        MWWorld::CellStore* mStore;
    };
    class GObject : public Object
    {
        using Object::Object;
        void updatePtr() const final { mPtr = mObjectRegistry->getPtr(mId, false); }
        sol::object getObject(lua_State* lua, ObjectId id) const final { return sol::make_object<GObject>(lua, id, mObjectRegistry); }
        sol::object getCell(lua_State* lua, MWWorld::CellStore* store) const final { return sol::make_object(lua, GCell{store}); }
    };

    using ObjectIdList = std::shared_ptr<std::vector<ObjectId>>;
    template <typename Obj>
    struct ObjectList { ObjectIdList mIds; };
    using GObjectList = ObjectList<GObject>;
    using LObjectList = ObjectList<LObject>;

    template <typename Obj>
    struct Inventory { Obj mObj; };

}

#endif  // MWLUA_OBJECT_H

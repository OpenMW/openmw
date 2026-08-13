#ifndef MWLUA_OBJECT_H
#define MWLUA_OBJECT_H

#include <stdexcept>

#include <sol/sol.hpp>

#include <components/esm3/cellref.hpp>

#include "../mwworld/ptr.hpp"

namespace MWLua
{
    // ObjectId is a unique identifier of a game object.
    // It can change only if the order of content files was change.
    using ObjectId = ESM::RefNum;
    inline ObjectId getId(const MWWorld::Ptr& ptr)
    {
        return ptr.getCellRef().getRefNum();
    }

    // Lua scripts can't use MWWorld::Ptr directly, because lifetime of a script can be longer than lifetime of Ptr.
    // `GObject` and `LObject` are intended to be passed to Lua as a userdata.
    // It automatically updates the underlying Ptr when needed.
    class Object : public MWWorld::SafePtr
    {
    public:
        using SafePtr::SafePtr;
        const MWWorld::Ptr& ptr() const
        {
            const MWWorld::Ptr& res = ptrOrEmpty();
            if (res.isEmpty())
                throw std::runtime_error("Object is not available: " + id().toString());
            return res;
        }

        virtual bool isLObject() const { return false; }
        virtual bool isGObject() const { return false; }
        virtual bool isSelfObject() const { return false; }
    };

    // Used only in local scripts
    struct LCell
    {
        MWWorld::CellStore* mStore;
    };
    class LObject : public Object
    {
        using Object::Object;

        bool isLObject() const override { return true; }
    };

    // Used only in global scripts
    struct GCell
    {
        MWWorld::CellStore* mStore;
    };
    class GObject : public Object
    {
        using Object::Object;

        bool isGObject() const override { return true; }
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

    template <typename Obj>
    struct Owner
    {
        Obj mObj;
    };

    // Repeated pushes of the same object return one userdata instead of allocating per access.
    int pushCachedObject(lua_State* state, const LObject& value);
    int pushCachedObject(lua_State* state, const GObject& value);
    int pushCachedObject(lua_State* state, const LCell& value);
    int pushCachedObject(lua_State* state, const GCell& value);

    // Must be called when the world is torn down: the Lua state survives it.
    void clearObjectCaches(lua_State* state);
}

namespace sol
{
    namespace stack
    {
        template <>
        struct unqualified_pusher<MWLua::LObject>
        {
            static int push(lua_State* state, const MWLua::LObject& value)
            {
                return MWLua::pushCachedObject(state, value);
            }
        };
        template <>
        struct unqualified_pusher<MWLua::GObject>
        {
            static int push(lua_State* state, const MWLua::GObject& value)
            {
                return MWLua::pushCachedObject(state, value);
            }
        };
        template <>
        struct unqualified_pusher<MWLua::LCell>
        {
            static int push(lua_State* state, const MWLua::LCell& value)
            {
                return MWLua::pushCachedObject(state, value);
            }
        };
        template <>
        struct unqualified_pusher<MWLua::GCell>
        {
            static int push(lua_State* state, const MWLua::GCell& value)
            {
                return MWLua::pushCachedObject(state, value);
            }
        };
    }
}

#endif // MWLUA_OBJECT_H

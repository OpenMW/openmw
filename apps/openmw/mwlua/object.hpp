#ifndef MWLUA_OBJECT_H
#define MWLUA_OBJECT_H

#include <map>
#include <stdexcept>
#include <typeindex>

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
    };

    // Used only in local scripts
    struct LCell
    {
        MWWorld::CellStore* mStore;
    };
    class LObject : public Object
    {
        using Object::Object;
    };

    // Used only in global scripts
    struct GCell
    {
        MWWorld::CellStore* mStore;
    };
    class GObject : public Object
    {
        using Object::Object;
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
}

#endif // MWLUA_OBJECT_H

#ifndef MWLUA_OBJECT_H
#define MWLUA_OBJECT_H

#include <components/esm/cellref.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"

namespace MWLua
{
    // ObjectId is a unique identifier of a game object.
    // It can change only if the order of content files was change.
    using ObjectId = ESM::RefNum;
    inline const ObjectId& getId(const MWWorld::Ptr& ptr) { return ptr.getCellRef().getRefNum(); }
    std::string idToString(const ObjectId& id);

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
        // If onlyActive = true, returns non-empty ptr only if it is registered and is in an active cell.
        // If onlyActive = false, tries to load and register the object if it is not loaded yet.
        // NOTE: `onlyActive` logic is not yet implemented.
        MWWorld::Ptr getPtr(ObjectId id, bool onlyActive);

        // Needed only for saving/loading.
        const ObjectId& getLastAssignedId() const { return mLastAssignedId; }
        void setLastAssignedId(ObjectId id) { mLastAssignedId = id; }

    private:
        friend class Object;
        friend class GObject;
        friend class LObject;

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
        std::string_view type() const;

        // Updates and returns the underlying Ptr. Throws an exception if object is not available.
        const MWWorld::Ptr& ptr() const;

        // Returns `true` if calling `ptr()` is safe.
        bool isValid() const;

    protected:
        virtual void updatePtr() const = 0;

        const ObjectId mId;
        ObjectRegistry* mObjectRegistry;

        mutable MWWorld::Ptr mPtr;
        mutable int64_t mLastUpdate = -1;
    };

    // Used only in local scripts
    class LObject : public Object
    {
        using Object::Object;
        void updatePtr() const final { mPtr = mObjectRegistry->getPtr(mId, true); }
    };

    // Used only in global scripts
    class GObject : public Object
    {
        using Object::Object;
        void updatePtr() const final { mPtr = mObjectRegistry->getPtr(mId, false); }
    };

    using ObjectIdList = std::shared_ptr<std::vector<ObjectId>>;
    template <typename Obj>
    struct ObjectList { ObjectIdList mIds; };
    using GObjectList = ObjectList<GObject>;
    using LObjectList = ObjectList<LObject>;

}

#endif  // MWLUA_OBJECT_H

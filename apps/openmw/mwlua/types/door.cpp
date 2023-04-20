#include "types.hpp"

#include <components/esm3/loaddoor.hpp>
#include <components/esm4/loaddoor.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"
#include "../worldview.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Door> : std::false_type
    {
    };

    template <>
    struct is_automagical<ESM4::Door> : std::false_type
    {
    };
}

namespace MWLua
{

    static const MWWorld::Ptr& doorPtr(const Object& o)
    {
        return verifyType(ESM::REC_DOOR, o.ptr());
    }

    static const MWWorld::Ptr& door4Ptr(const Object& o)
    {
        return verifyType(ESM::REC_DOOR4, o.ptr());
    }

    void addDoorBindings(sol::table door, const Context& context)
    {
        door["isTeleport"] = [](const Object& o) { return doorPtr(o).getCellRef().getTeleport(); };
        door["destPosition"]
            = [](const Object& o) -> osg::Vec3f { return doorPtr(o).getCellRef().getDoorDest().asVec3(); };
        door["destRotation"]
            = [](const Object& o) -> osg::Vec3f { return doorPtr(o).getCellRef().getDoorDest().asRotationVec3(); };
        door["destCell"] = [](sol::this_state lua, const Object& o) -> sol::object {
            const MWWorld::CellRef& cellRef = doorPtr(o).getCellRef();
            if (!cellRef.getTeleport())
                return sol::nil;
            MWWorld::CellStore* cell = MWBase::Environment::get().getWorldModel()->getCell(cellRef.getDestCell());
            assert(cell);
            return o.getCell(lua, cell);
        };

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Door>(door, context);

        sol::usertype<ESM::Door> record = context.mLua->sol().new_usertype<ESM::Door>("ESM3_Door");
        record[sol::meta_function::to_string]
            = [](const ESM::Door& rec) -> std::string { return "ESM3_Door[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Door& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mScript.serializeText(); });
        record["openSound"] = sol::readonly_property(
            [](const ESM::Door& rec) -> std::string { return rec.mOpenSound.serializeText(); });
        record["closeSound"] = sol::readonly_property(
            [](const ESM::Door& rec) -> std::string { return rec.mCloseSound.serializeText(); });
    }

    void addESM4DoorBindings(sol::table door, const Context& context)
    {
        door["isTeleport"] = [](const Object& o) { return door4Ptr(o).getCellRef().getTeleport(); };
        door["destPosition"]
            = [](const Object& o) -> osg::Vec3f { return door4Ptr(o).getCellRef().getDoorDest().asVec3(); };
        door["destRotation"]
            = [](const Object& o) -> osg::Vec3f { return door4Ptr(o).getCellRef().getDoorDest().asRotationVec3(); };
        door["destCell"] = [](sol::this_state lua, const Object& o) -> sol::object {
            const MWWorld::CellRef& cellRef = door4Ptr(o).getCellRef();
            if (!cellRef.getTeleport())
                return sol::nil;
            MWWorld::CellStore* cell = MWBase::Environment::get().getWorldModel()->getCell(cellRef.getDestCell());
            assert(cell);
            return o.getCell(lua, cell);
        };

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM4::Door>(door, context, "ESM4Door");

        sol::usertype<ESM4::Door> record = context.mLua->sol().new_usertype<ESM4::Door>("ESM4_Door");
        record[sol::meta_function::to_string]
            = [](const ESM4::Door& rec) -> std::string { return "ESM4_Door[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM4::Door& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM4::Door& rec) -> std::string { return rec.mFullName; });
        record["model"] = sol::readonly_property([vfs](const ESM4::Door& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
    }
}

#include "types.hpp"

#include "modelproperty.hpp"

#include "../localscripts.hpp"

#include <components/esm3/loaddoor.hpp>
#include <components/esm4/loaddoor.hpp>
#include <components/lua/util.hpp>
#include <components/lua/utilpackage.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwworld/class.hpp"
#include "apps/openmw/mwworld/worldmodel.hpp"

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
        sol::state_view lua = context.sol();
        door["STATE"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, MWWorld::DoorState>(lua,
            {
                { "Idle", MWWorld::DoorState::Idle },
                { "Opening", MWWorld::DoorState::Opening },
                { "Closing", MWWorld::DoorState::Closing },
            }));
        door["getDoorState"] = [](const Object& o) -> MWWorld::DoorState {
            const MWWorld::Ptr& ptr = doorPtr(o);
            return ptr.getClass().getDoorState(ptr);
        };
        door["isOpen"] = [](const Object& o) {
            const MWWorld::Ptr& ptr = doorPtr(o);
            bool doorIsIdle = ptr.getClass().getDoorState(ptr) == MWWorld::DoorState::Idle;
            bool doorIsOpen = ptr.getRefData().getPosition().rot[2] != ptr.getCellRef().getPosition().rot[2];

            return doorIsIdle && doorIsOpen;
        };
        door["isClosed"] = [](const Object& o) {
            const MWWorld::Ptr& ptr = doorPtr(o);
            bool doorIsIdle = ptr.getClass().getDoorState(ptr) == MWWorld::DoorState::Idle;
            bool doorIsOpen = ptr.getRefData().getPosition().rot[2] != ptr.getCellRef().getPosition().rot[2];

            return doorIsIdle && !doorIsOpen;
        };
        door["activateDoor"] = [](const Object& o, sol::optional<bool> openState) {
            bool allowChanges
                = dynamic_cast<const GObject*>(&o) != nullptr || dynamic_cast<const SelfObject*>(&o) != nullptr;
            if (!allowChanges)
                throw std::runtime_error("Can only be used in global scripts or in local scripts on self.");

            const MWWorld::Ptr& ptr = doorPtr(o);
            auto world = MWBase::Environment::get().getWorld();

            if (!openState.has_value())
                world->activateDoor(ptr);
            else if (*openState)
                world->activateDoor(ptr, MWWorld::DoorState::Opening);
            else
                world->activateDoor(ptr, MWWorld::DoorState::Closing);
        };
        door["isTeleport"] = [](const Object& o) { return doorPtr(o).getCellRef().getTeleport(); };
        door["destPosition"]
            = [](const Object& o) -> osg::Vec3f { return doorPtr(o).getCellRef().getDoorDest().asVec3(); };
        door["destRotation"] = [](const Object& o) -> LuaUtil::TransformQ {
            return { Misc::Convert::makeOsgQuat(doorPtr(o).getCellRef().getDoorDest().rot) };
        };
        door["destCell"] = [](sol::this_state thisState, const Object& o) -> sol::object {
            const MWWorld::CellRef& cellRef = doorPtr(o).getCellRef();
            if (!cellRef.getTeleport())
                return sol::nil;
            MWWorld::CellStore& cell = MWBase::Environment::get().getWorldModel()->getCell(cellRef.getDestCell());
            if (dynamic_cast<const GObject*>(&o))
                return sol::make_object(thisState, GCell{ &cell });
            else
                return sol::make_object(thisState, LCell{ &cell });
        };

        addRecordFunctionBinding<ESM::Door>(door, context);

        sol::usertype<ESM::Door> record = lua.new_usertype<ESM::Door>("ESM3_Door");
        record[sol::meta_function::to_string]
            = [](const ESM::Door& rec) -> std::string { return "ESM3_Door[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["mwscript"] = sol::readonly_property([](const ESM::Door& rec) -> ESM::RefId { return rec.mScript; });
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
        door["destRotation"] = [](const Object& o) -> LuaUtil::TransformQ {
            return { Misc::Convert::makeOsgQuat(door4Ptr(o).getCellRef().getDoorDest().rot) };
        };
        door["destCell"] = [](sol::this_state lua, const Object& o) -> sol::object {
            const MWWorld::CellRef& cellRef = door4Ptr(o).getCellRef();
            if (!cellRef.getTeleport())
                return sol::nil;
            MWWorld::CellStore& cell = MWBase::Environment::get().getWorldModel()->getCell(cellRef.getDestCell());
            if (dynamic_cast<const GObject*>(&o))
                return sol::make_object(lua, GCell{ &cell });
            else
                return sol::make_object(lua, LCell{ &cell });
        };

        addRecordFunctionBinding<ESM4::Door>(door, context, "ESM4Door");

        sol::usertype<ESM4::Door> record = context.sol().new_usertype<ESM4::Door>("ESM4_Door");
        record[sol::meta_function::to_string] = [](const ESM4::Door& rec) -> std::string {
            return "ESM4_Door[" + ESM::RefId(rec.mId).toDebugString() + "]";
        };
        record["id"] = sol::readonly_property(
            [](const ESM4::Door& rec) -> std::string { return ESM::RefId(rec.mId).serializeText(); });
        record["name"] = sol::readonly_property([](const ESM4::Door& rec) -> std::string { return rec.mFullName; });
        addModelProperty(record);
        record["isAutomatic"] = sol::readonly_property(
            [](const ESM4::Door& rec) -> bool { return rec.mDoorFlags & ESM4::Door::Flag_AutomaticDoor; });
        record["openSound"] = sol::readonly_property(
            [](const ESM4::Door& rec) -> std::string { return ESM::RefId(rec.mOpenSound).serializeText(); });
        record["closeSound"] = sol::readonly_property(
            [](const ESM4::Door& rec) -> std::string { return ESM::RefId(rec.mCloseSound).serializeText(); });
    }
}
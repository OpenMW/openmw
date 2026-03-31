#include "types.hpp"
#include "usertypeutil.hpp"

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
    namespace
    {
        void addPropertyFromTable(const sol::table& rec, std::string_view key, ESM::RefId& value)
        {
            if (rec[key] != sol::nil)
            {
                std::string_view id = rec[key].get<std::string_view>();
                value = ESM::RefId::deserializeText(id);
            }
        }

        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Door[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Door::mName);
            Types::addModelProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Door::mScript);
            Types::addProperty(record, "openSound", &ESM::Door::mOpenSound);
            Types::addProperty(record, "closeSound", &ESM::Door::mCloseSound);
        }
    }

    ESM::Door tableToDoor(const sol::table& rec)
    {
        auto door = Types::initFromTemplate<ESM::Door>(rec);

        if (rec["name"] != sol::nil)
            door.mName = rec["name"];
        if (rec["model"] != sol::nil)
            door.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        addPropertyFromTable(rec, "mwscript", door.mScript);
        addPropertyFromTable(rec, "openSound", door.mOpenSound);
        addPropertyFromTable(rec, "closeSound", door.mCloseSound);

        return door;
    }

    static const MWWorld::Ptr& doorPtr(const Object& o)
    {
        return verifyType(ESM::REC_DOOR, o.ptr());
    }

    static const MWWorld::Ptr& door4Ptr(const Object& o)
    {
        return verifyType(ESM::REC_DOOR4, o.ptr());
    }

    void addMutableDoorType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Door>>(lua, "ESM3_MutableDoor");
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
        door["createRecordDraft"] = tableToDoor;
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
            bool allowChanges = o.isGObject() || o.isSelfObject();
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
            if (o.isGObject())
                return sol::make_object(thisState, GCell{ &cell });
            else
                return sol::make_object(thisState, LCell{ &cell });
        };

        addRecordFunctionBinding<ESM::Door>(door, context);

        addUserType<ESM::Door>(lua, "ESM3_Door");
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
            if (o.isGObject())
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

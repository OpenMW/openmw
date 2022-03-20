#include "types.hpp"

#include <components/esm3/loaddoor.hpp>

#include <apps/openmw/mwworld/esmstore.hpp>

#include "../luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Door> : std::false_type {};
}

namespace MWLua
{

    static const MWWorld::Ptr& doorPtr(const Object& o) { return verifyType(ESM::REC_DOOR, o.ptr()); }

    void addDoorBindings(sol::table door, const Context& context)
    {
        door["isTeleport"] = [](const Object& o) { return doorPtr(o).getCellRef().getTeleport(); };
        door["destPosition"] = [](const Object& o) -> osg::Vec3f
        {
            return doorPtr(o).getCellRef().getDoorDest().asVec3();
        };
        door["destRotation"] = [](const Object& o) -> osg::Vec3f
        {
            return doorPtr(o).getCellRef().getDoorDest().asRotationVec3();
        };
        door["destCell"] = [worldView=context.mWorldView](sol::this_state lua, const Object& o) -> sol::object
        {
            const MWWorld::CellRef& cellRef = doorPtr(o).getCellRef();
            if (!cellRef.getTeleport())
                return sol::nil;
            MWWorld::CellStore* cell = worldView->findCell(cellRef.getDestCell(), cellRef.getDoorDest().asVec3());
            if (cell)
                return o.getCell(lua, cell);
            else
                return sol::nil;
        };

        const MWWorld::Store<ESM::Door>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Door>();
        door["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Door* { return obj.ptr().get<ESM::Door>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Door* { return store->find(recordId); });
        sol::usertype<ESM::Door> record = context.mLua->sol().new_usertype<ESM::Door>("ESM3_Door");
        record[sol::meta_function::to_string] = sol::readonly_property(
            [](const ESM::Door& rec) -> std::string { return "ESM3_Door[" + rec.mId + "]"; });
        record["id"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mId; });
        record["name"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mModel; });
        record["mwscript"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mScript; });
        record["openSound"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mOpenSound; });
        record["closeSound"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mCloseSound; });
    }

}

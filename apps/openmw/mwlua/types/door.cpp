#include "types.hpp"

#include <components/esm3/loaddoor.hpp>
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
}

namespace MWLua
{

    static const MWWorld::Ptr& doorPtr(const Object& o)
    {
        return verifyType(ESM::REC_DOOR, o.ptr());
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
            MWWorld::CellStore* cell = MWBase::Environment::get().getWorldModel()->getCellByPosition(
                cellRef.getDoorDest().asVec3(), cellRef.getDestCell());
            assert(cell);
            return o.getCell(lua, cell);
        };

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        const MWWorld::Store<ESM::Door>* store = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Door>();
        door["record"]
            = sol::overload([](const Object& obj) -> const ESM::Door* { return obj.ptr().get<ESM::Door>()->mBase; },
                [store](const std::string& recordId) -> const ESM::Door* {
                    return store->find(ESM::RefId::stringRefId(recordId));
                });
        sol::usertype<ESM::Door> record = context.mLua->sol().new_usertype<ESM::Door>("ESM3_Door");
        record[sol::meta_function::to_string]
            = [](const ESM::Door& rec) -> std::string { return "ESM3_Door[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mId.getRefIdString(); });
        record["name"] = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Door& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Door& rec) -> std::string { return rec.mScript.getRefIdString(); });
        record["openSound"] = sol::readonly_property(
            [](const ESM::Door& rec) -> std::string { return rec.mOpenSound.getRefIdString(); });
        record["closeSound"] = sol::readonly_property(
            [](const ESM::Door& rec) -> std::string { return rec.mCloseSound.getRefIdString(); });
    }

}

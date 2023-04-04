#include "types.hpp"

#include <components/esm3/loadrepa.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Repair> : std::false_type
    {
    };
}

namespace MWLua
{
    void addRepairBindings(sol::table repair, const Context& context)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Repair>(repair);

        sol::usertype<ESM::Repair> record = context.mLua->sol().new_usertype<ESM::Repair>("ESM3_Repair");
        record[sol::meta_function::to_string]
            = [](const ESM::Repair& rec) { return "ESM3_Repair[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Repair& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Repair& rec) -> std::string { return rec.mScript.serializeText(); });
        record["icon"] = sol::readonly_property([vfs](const ESM::Repair& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["maxCondition"] = sol::readonly_property([](const ESM::Repair& rec) -> int { return rec.mData.mUses; });
        record["value"] = sol::readonly_property([](const ESM::Repair& rec) -> int { return rec.mData.mValue; });
        record["weight"] = sol::readonly_property([](const ESM::Repair& rec) -> float { return rec.mData.mWeight; });
        record["quality"] = sol::readonly_property([](const ESM::Repair& rec) -> float { return rec.mData.mQuality; });
    }
}

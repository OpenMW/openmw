#include "types.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Potion> : std::false_type
    {
    };
}

namespace MWLua
{
    void addPotionBindings(sol::table potion, const Context& context)
    {
        addRecordFunctionBinding<ESM::Potion>(potion);

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        sol::usertype<ESM::Potion> record = context.mLua->sol().new_usertype<ESM::Potion>("ESM3_Potion");
        record[sol::meta_function::to_string]
            = [](const ESM::Potion& rec) { return "ESM3_Potion[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mId.getRefIdString(); });
        record["name"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Potion& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Potion& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Potion& rec) -> std::string { return rec.mScript.getRefIdString(); });
        record["weight"] = sol::readonly_property([](const ESM::Potion& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Potion& rec) -> int { return rec.mData.mValue; });
    }
}

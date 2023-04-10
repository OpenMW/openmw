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

namespace
{
    // Populates a potion struct from a Lua table.
    ESM::Potion tableToPotion(const sol::table& rec)
    {
        ESM::Potion potion;
        potion.mName = rec["name"];
        potion.mModel = rec["model"];
        potion.mIcon = rec["icon"];
        std::string_view scriptId = rec["mwscript"].get<std::string_view>();
        potion.mScript = ESM::RefId::deserializeText(scriptId);
        potion.mData.mWeight = rec["weight"];
        potion.mData.mValue = rec["value"];

        // Note: The list of effects is not yet present in openmw.types.Potion,
        // so we don't map it here either.
        return potion;
    }
}

namespace MWLua
{
    void addPotionBindings(sol::table potion, const Context& context)
    {
        addRecordFunctionBinding<ESM::Potion>(potion, context);

        // Creates a new potion struct but does not store it in MWWorld::ESMStore.
        // Global scripts can use world.createRecord to add the potion to the world.
        // Note: This potion instance must be owned by lua, so we return it
        // by value.
        potion["createRecordDraft"] = tableToPotion;

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        sol::usertype<ESM::Potion> record = context.mLua->sol().new_usertype<ESM::Potion>("ESM3_Potion");
        record[sol::meta_function::to_string]
            = [](const ESM::Potion& rec) { return "ESM3_Potion[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Potion& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Potion& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mScript.serializeText(); });
        record["weight"] = sol::readonly_property([](const ESM::Potion& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Potion& rec) -> int { return rec.mData.mValue; });
    }
}

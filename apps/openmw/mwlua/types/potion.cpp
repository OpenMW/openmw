#include "types.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"

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
        if (rec["template"] != sol::nil)
            potion = LuaUtil::cast<ESM::Potion>(rec["template"]);
        else
            potion.blank();
        if (rec["name"] != sol::nil)
            potion.mName = rec["name"];
        if (rec["model"] != sol::nil)
            potion.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            potion.mIcon = rec["icon"];
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            potion.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["weight"] != sol::nil)
            potion.mData.mWeight = rec["weight"];
        if (rec["value"] != sol::nil)
            potion.mData.mValue = rec["value"];
        if (rec["effects"] != sol::nil)
        {
            sol::table effectsTable = rec["effects"];
            size_t numEffects = effectsTable.size();
            potion.mEffects.mList.resize(numEffects);
            for (size_t i = 0; i < numEffects; ++i)
            {
                potion.mEffects.mList[i] = LuaUtil::cast<ESM::IndexedENAMstruct>(effectsTable[LuaUtil::toLuaIndex(i)]);
            }
            potion.mEffects.updateIndexes();
        }
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
        record["model"] = sol::readonly_property(
            [](const ESM::Potion& rec) -> std::string { return Misc::ResourceHelpers::correctMeshPath(rec.mModel); });
        record["icon"] = sol::readonly_property([vfs](const ESM::Potion& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mScript.serializeText(); });
        record["weight"] = sol::readonly_property([](const ESM::Potion& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Potion& rec) -> int { return rec.mData.mValue; });
        record["effects"] = sol::readonly_property([context](const ESM::Potion& rec) -> sol::table {
            sol::table res(context.mLua->sol(), sol::create);
            for (size_t i = 0; i < rec.mEffects.mList.size(); ++i)
                res[LuaUtil::toLuaIndex(i)] = rec.mEffects.mList[i]; // ESM::IndexedENAMstruct (effect params)
            return res;
        });
    }
}

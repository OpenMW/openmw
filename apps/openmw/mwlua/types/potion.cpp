#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadmgef.hpp>
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
                sol::object element = effectsTable[LuaUtil::toLuaIndex(i)];
                if (element.is<ESM::IndexedENAMstruct>()) // It can be casted (extracted from another magic thing)
                {
                    potion.mEffects.mList[i]
                        = LuaUtil::cast<ESM::IndexedENAMstruct>(effectsTable[LuaUtil::toLuaIndex(i)]);
                }
                else // Recreate from a table
                {
                    ESM::IndexedENAMstruct effect;
                    effect.blank();
                    sol::table effectTable = effectsTable[LuaUtil::toLuaIndex(i)];
                    if (effectTable["id"] != sol::nil)
                        effect.mData.mEffectID = ESM::MagicEffect::indexNameToIndex(effectTable["id"].get<std::string_view>());
                    if (effectTable["affectedSkill"] != sol::nil)
                        effect.mData.mSkill = ESM::Skill::refIdToIndex(
                            ESM::RefId::deserializeText(effectTable["affectedSkill"].get<std::string_view>()));
                    if (effectTable["affectedAttribute"] != sol::nil)
                        effect.mData.mAttribute = ESM::Attribute::refIdToIndex(
                            ESM::RefId::deserializeText(effectTable["affectedAttribute"].get<std::string_view>()));
                    if (effectTable["range"] != sol::nil)
                        effect.mData.mRange = effectTable["range"].get<int32_t>();
                    if (effectTable["area"] != sol::nil)
                        effect.mData.mArea = effectTable["area"].get<int32_t>();
                    if (effectTable["duration"] != sol::nil)
                        effect.mData.mDuration = effectTable["duration"].get<int32_t>();
                    if (effectTable["magnitudeMin"] != sol::nil)
                        effect.mData.mMagnMin = effectTable["magnitudeMin"].get<int32_t>();
                    if (effectTable["magnitudeMax"] != sol::nil)
                        effect.mData.mMagnMax = effectTable["magnitudeMax"].get<int32_t>();
                    potion.mEffects.mList[i] = effect;
                }
            } // Indexes are updated automatically so we do not care when creating a new table
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
        sol::state_view lua = context.sol();
        sol::usertype<ESM::Potion> record = lua.new_usertype<ESM::Potion>("ESM3_Potion");
        record[sol::meta_function::to_string]
            = [](const ESM::Potion& rec) { return "ESM3_Potion[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Potion& rec) -> std::string { return rec.mName; });
        addModelProperty(record);
        record["icon"] = sol::readonly_property([vfs](const ESM::Potion& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Potion& rec) -> sol::optional<std::string> { return LuaUtil::serializeRefId(rec.mScript); });
        record["weight"] = sol::readonly_property([](const ESM::Potion& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Potion& rec) -> int { return rec.mData.mValue; });
        record["effects"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Potion& rec) -> sol::table {
            sol::table res(lua, sol::create);
            for (size_t i = 0; i < rec.mEffects.mList.size(); ++i)
                res[LuaUtil::toLuaIndex(i)] = rec.mEffects.mList[i]; // ESM::IndexedENAMstruct (effect params)
            return res;
        });
    }
}

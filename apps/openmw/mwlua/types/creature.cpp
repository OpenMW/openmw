#include "../stats.hpp"
#include "actor.hpp"
#include "types.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Creature> : std::false_type
    {
    };
}

namespace MWLua
{
    void addCreatureBindings(sol::table creature, const Context& context)
    {
        creature["TYPE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            { "Creatures", ESM::Creature::Creatures },
            { "Daedra", ESM::Creature::Daedra },
            { "Undead", ESM::Creature::Undead },
            { "Humanoid", ESM::Creature::Humanoid },
        }));

        addRecordFunctionBinding<ESM::Creature>(creature, context);

        sol::usertype<ESM::Creature> record = context.mLua->sol().new_usertype<ESM::Creature>("ESM3_Creature");
        record[sol::meta_function::to_string]
            = [](const ESM::Creature& rec) { return "ESM3_Creature[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return Misc::ResourceHelpers::correctMeshPath(rec.mModel); });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return rec.mScript.serializeText(); });
        record["baseCreature"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return rec.mOriginal.serializeText(); });
        record["soulValue"] = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mSoul; });
        record["type"] = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mType; });
        record["baseGold"] = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mGold; });
        record["combatSkill"]
            = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mCombat; });
        record["magicSkill"] = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mMagic; });
        record["stealthSkill"]
            = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mStealth; });
        record["attack"] = sol::readonly_property([context](const ESM::Creature& rec) -> sol::table {
            sol::state_view& lua = context.mLua->sol();
            sol::table res(lua, sol::create);
            int index = 1;
            for (auto attack : rec.mData.mAttack)
                res[index++] = attack;
            return LuaUtil::makeReadOnly(res);
        });

        addActorServicesBindings<ESM::Creature>(record, context);
    }
}

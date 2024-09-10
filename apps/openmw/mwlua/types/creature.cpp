#include "../stats.hpp"
#include "actor.hpp"
#include "types.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
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
        sol::state_view lua = context.sol();
        creature["TYPE"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(lua,
            {
                { "Creatures", ESM::Creature::Creatures },
                { "Daedra", ESM::Creature::Daedra },
                { "Undead", ESM::Creature::Undead },
                { "Humanoid", ESM::Creature::Humanoid },
            }));

        addRecordFunctionBinding<ESM::Creature>(creature, context);

        sol::usertype<ESM::Creature> record = lua.new_usertype<ESM::Creature>("ESM3_Creature");
        record[sol::meta_function::to_string]
            = [](const ESM::Creature& rec) { return "ESM3_Creature[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return Misc::ResourceHelpers::correctMeshPath(rec.mModel); });
        record["mwscript"] = sol::readonly_property([](const ESM::Creature& rec) -> sol::optional<std::string> {
            return LuaUtil::serializeRefId(rec.mScript);
        });
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
        record["attack"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Creature& rec) -> sol::table {
            sol::table res(lua, sol::create);
            int index = 1;
            for (auto attack : rec.mData.mAttack)
                res[index++] = attack;
            return LuaUtil::makeReadOnly(res);
        });
        record["canFly"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> bool { return rec.mFlags & ESM::Creature::Flies; });
        record["canSwim"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> bool { return rec.mFlags & ESM::Creature::Swims; });
        record["canUseWeapons"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> bool { return rec.mFlags & ESM::Creature::Weapon; });
        record["canWalk"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> bool { return rec.mFlags & ESM::Creature::Walks; });
        record["isBiped"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> bool { return rec.mFlags & ESM::Creature::Bipedal; });
        record["isEssential"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> bool { return rec.mFlags & ESM::Creature::Essential; });
        record["isRespawning"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> bool { return rec.mFlags & ESM::Creature::Respawn; });

        addActorServicesBindings<ESM::Creature>(record, context);
    }
}

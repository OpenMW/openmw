#include "types.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

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

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Creature>(creature, context);

        sol::usertype<ESM::Creature> record = context.mLua->sol().new_usertype<ESM::Creature>("ESM3_Creature");
        record[sol::meta_function::to_string]
            = [](const ESM::Creature& rec) { return "ESM3_Creature[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Creature& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Creature& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return rec.mScript.serializeText(); });
        record["baseCreature"] = sol::readonly_property(
            [](const ESM::Creature& rec) -> std::string { return rec.mOriginal.serializeText(); });
        record["soulValue"] = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mSoul; });
        record["type"] = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mType; });
        record["baseGold"] = sol::readonly_property([](const ESM::Creature& rec) -> int { return rec.mData.mGold; });
        record["servicesOffered"] = sol::readonly_property([](const ESM::Creature& rec) -> std::vector<std::string> {
            std::vector<std::string> providedServices;
            std::map<int, std::string> serviceNames = { { ESM::NPC::Spells, "Spells" },
                { ESM::NPC::Spellmaking, "Spellmaking" }, { ESM::NPC::Enchanting, "Enchanting" },
                { ESM::NPC::Training, "Training" }, { ESM::NPC::Repair, "Repair" }, { ESM::NPC::AllItems, "Barter" },
                { ESM::NPC::Weapon, "Weapon" }, { ESM::NPC::Armor, "Armor" }, { ESM::NPC::Clothing, "Clothing" },
                { ESM::NPC::Books, "Books" }, { ESM::NPC::Ingredients, "Ingredients" }, { ESM::NPC::Picks, "Picks" },
                { ESM::NPC::Probes, "Probes" }, { ESM::NPC::Lights, "Lights" }, { ESM::NPC::Apparatus, "Apparatus" },
                { ESM::NPC::RepairItem, "RepairItem" }, { ESM::NPC::Misc, "Misc" }, { ESM::NPC::Potions, "Potions" },
                { ESM::NPC::MagicItems, "MagicItems" } };

            int mServices = rec.mAiData.mServices;
            for (const auto& entry : serviceNames)
            {
                if (mServices & entry.first)
                {
                    providedServices.push_back(entry.second);
                }
            }
            if (!rec.getTransport().empty())
                providedServices.push_back("Travel");
            return providedServices;
        });
    }
}

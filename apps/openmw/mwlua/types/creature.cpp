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
        record["servicesOffered"] = sol::readonly_property([](const ESM::Creature& rec) {
            std::vector<std::string> providedServices;

            int mServices = rec.mAiData.mServices;
            if (mServices & ESM::NPC::Spells)
                providedServices.push_back("Spells");
            if (mServices & ESM::NPC::Spellmaking)
                providedServices.push_back("Spellmaking");
            if (mServices & ESM::NPC::Enchanting)
                providedServices.push_back("Enchanting");
            if (mServices & ESM::NPC::Repair)
                providedServices.push_back("Repair");
            if (mServices & ESM::NPC::AllItems)
                providedServices.push_back("Barter");

            if (mServices & ESM::NPC::Weapon)
                providedServices.push_back("Weapon");
            if (mServices & ESM::NPC::Armor)
                providedServices.push_back("Armor");
            if (mServices & ESM::NPC::Clothing)
                providedServices.push_back("Clothing");
            if (mServices & ESM::NPC::Books)
                providedServices.push_back("Books");
            if (mServices & ESM::NPC::Ingredients)
                providedServices.push_back("Ingredients");
            if (mServices & ESM::NPC::Picks)
                providedServices.push_back("Picks");
            if (mServices & ESM::NPC::Probes)
                providedServices.push_back("Probes");
            if (mServices & ESM::NPC::Lights)
                providedServices.push_back("Lights");
            if (mServices & ESM::NPC::Apparatus)
                providedServices.push_back("Apparatus");
            if (mServices & ESM::NPC::RepairItem)
                providedServices.push_back("RepairItem");
            if (mServices & ESM::NPC::Misc)
                providedServices.push_back("Misc");
            if (mServices & ESM::NPC::Potions)
                providedServices.push_back("Potions");
            if (mServices & ESM::NPC::MagicItems)
                providedServices.push_back("MagicItems");
            if (rec.getTransport().size() > 0)
                providedServices.push_back("Travel");

            return providedServices;
        });
    }
}

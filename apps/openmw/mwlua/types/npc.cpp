#include "types.hpp"

#include "actor.hpp"
#include "modelproperty.hpp"

#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwbase/mechanicsmanager.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwmechanics/npcstats.hpp"
#include "apps/openmw/mwworld/class.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"

#include "../classbindings.hpp"
#include "../localscripts.hpp"
#include "../racebindings.hpp"
#include "../stats.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::NPC> : std::false_type
    {
    };
}

namespace
{
    ESM::NPC tableToNPC(const sol::table& rec)
    {
        ESM::NPC npc;

        // Start from template if provided
        if (rec["template"] != sol::nil)
            npc = LuaUtil::cast<ESM::NPC>(rec["template"]);
        else
            npc.blank();

        // Force dummy ID
        npc.mId = ESM::RefId::deserializeText("blank");

        // Basic fields
        if (rec["name"] != sol::nil)
            npc.mName = rec["name"];
        if (rec["model"] != sol::nil)
            npc.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["mwscript"] != sol::nil)
            npc.mScript = ESM::RefId::deserializeText(rec["mwscript"].get<std::string_view>());
        if (rec["race"] != sol::nil)
            npc.mRace = ESM::RefId::deserializeText(rec["race"].get<std::string_view>());
        if (rec["class"] != sol::nil)
            npc.mClass = ESM::RefId::deserializeText(rec["class"].get<std::string_view>());
        if (rec["head"] != sol::nil)
            npc.mHead = ESM::RefId::deserializeText(rec["head"].get<std::string_view>());
        if (rec["hair"] != sol::nil)
            npc.mHair = ESM::RefId::deserializeText(rec["hair"].get<std::string_view>());

        if (rec["isMale"] != sol::nil)
        {
            bool male = rec["isMale"];
            if (male)
                npc.mFlags &= ~ESM::NPC::Female;
            else
                npc.mFlags |= ESM::NPC::Female;
        }

        if (rec["isEssential"] != sol::nil)
        {
            bool essential = rec["isEssential"];
            if (essential)
                npc.mFlags |= ESM::NPC::Essential;
            else
                npc.mFlags &= ~ESM::NPC::Essential;
        }

        if (rec["isRespawning"] != sol::nil)
        {
            bool respawn = rec["isRespawning"];
            if (respawn)
                npc.mFlags |= ESM::NPC::Respawn;
            else
                npc.mFlags &= ~ESM::NPC::Respawn;
        }

        if (rec["baseDisposition"] != sol::nil)
            npc.mNpdt.mDisposition = static_cast<int>(rec["baseDisposition"]);

        if (rec["baseGold"] != sol::nil)
            npc.mNpdt.mGold = static_cast<int>(rec["baseGold"]);

        if (rec["bloodType"] != sol::nil)
            npc.mBloodType = static_cast<int>(rec["bloodType"]);

        // Services offered
        if (rec["servicesOffered"] != sol::nil)
        {
            const sol::table services = rec["servicesOffered"];
            int flags = 0;
            auto setFlag = [&](const char* key, int mask) {
                if (services[key] != sol::nil && services[key])
                    flags |= mask;
            };

            setFlag("Spells", ESM::NPC::Spells);
            setFlag("Spellmaking", ESM::NPC::Spellmaking);
            setFlag("Enchanting", ESM::NPC::Enchanting);
            setFlag("Training", ESM::NPC::Training);
            setFlag("Repair", ESM::NPC::Repair);
            setFlag("Barter", ESM::NPC::AllItems);
            setFlag("Weapon", ESM::NPC::Weapon);
            setFlag("Armor", ESM::NPC::Armor);
            setFlag("Clothing", ESM::NPC::Clothing);
            setFlag("Books", ESM::NPC::Books);
            setFlag("Ingredients", ESM::NPC::Ingredients);
            setFlag("Picks", ESM::NPC::Picks);
            setFlag("Probes", ESM::NPC::Probes);
            setFlag("Lights", ESM::NPC::Lights);
            setFlag("Apparatus", ESM::NPC::Apparatus);
            setFlag("RepairItem", ESM::NPC::RepairItem);
            setFlag("Misc", ESM::NPC::Misc);
            setFlag("Potions", ESM::NPC::Potions);
            setFlag("MagicItems", ESM::NPC::MagicItems);

            npc.mAiData.mServices = flags;
        }

        return npc;
    }
}

namespace
{
    size_t getValidRanksCount(const ESM::Faction* faction)
    {
        if (!faction)
            return 0;

        for (size_t i = 0; i < faction->mRanks.size(); i++)
        {
            if (faction->mRanks[i].empty())
                return i;
        }

        return faction->mRanks.size();
    }

    ESM::RefId parseFactionId(std::string_view faction)
    {
        ESM::RefId id = ESM::RefId::deserializeText(faction);
        const MWWorld::ESMStore* store = MWBase::Environment::get().getESMStore();
        if (!store->get<ESM::Faction>().search(id))
            throw std::runtime_error("Faction '" + std::string(faction) + "' does not exist");
        return id;
    }

    void verifyPlayer(const MWLua::Object& o)
    {
        if (o.ptr() != MWBase::Environment::get().getWorld()->getPlayerPtr())
            throw std::runtime_error("The argument must be a player!");
    }

    void verifyNpc(const MWWorld::Class& cls)
    {
        if (!cls.isNpc())
            throw std::runtime_error("The argument must be a NPC!");
    }
}

namespace MWLua
{
    void addNpcBindings(sol::table npc, const Context& context)
    {
        addNpcStatsBindings(npc, context);

        addRecordFunctionBinding<ESM::NPC>(npc, context);

        sol::state_view lua = context.sol();

        sol::usertype<ESM::NPC> record = lua.new_usertype<ESM::NPC>("ESM3_NPC");
        record[sol::meta_function::to_string]
            = [](const ESM::NPC& rec) { return "ESM3_NPC[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mName; });
        record["race"]
            = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mRace.serializeText(); });
        record["class"]
            = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mClass.serializeText(); });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::NPC& rec) -> sol::optional<std::string> { return LuaUtil::serializeRefId(rec.mScript); });
        record["hair"]
            = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mHair.serializeText(); });
        record["baseDisposition"]
            = sol::readonly_property([](const ESM::NPC& rec) -> int { return (int)rec.mNpdt.mDisposition; });
        record["head"]
            = sol::readonly_property([](const ESM::NPC& rec) -> std::string { return rec.mHead.serializeText(); });
        addModelProperty(record);
        record["isEssential"]
            = sol::readonly_property([](const ESM::NPC& rec) -> bool { return rec.mFlags & ESM::NPC::Essential; });
        record["isMale"] = sol::readonly_property([](const ESM::NPC& rec) -> bool { return rec.isMale(); });
        record["isRespawning"]
            = sol::readonly_property([](const ESM::NPC& rec) -> bool { return rec.mFlags & ESM::NPC::Respawn; });
        record["baseGold"] = sol::readonly_property([](const ESM::NPC& rec) -> int { return rec.mNpdt.mGold; });
        record["bloodType"] = sol::readonly_property([](const ESM::NPC& rec) -> int { return rec.mBloodType; });
        addActorServicesBindings<ESM::NPC>(record, context);

        npc["classes"] = initClassRecordBindings(context);
        npc["races"] = initRaceRecordBindings(context);

        // This function is game-specific, in future we should replace it with something more universal.
        npc["isWerewolf"] = [](const Object& o) {
            const MWWorld::Class& cls = o.ptr().getClass();
            if (cls.isNpc())
                return cls.getNpcStats(o.ptr()).isWerewolf();
            else
                throw std::runtime_error("NPC or Player expected");
        };

        npc["getDisposition"] = [](const Object& o, const Object& player) -> int {
            const MWWorld::Class& cls = o.ptr().getClass();
            verifyPlayer(player);
            verifyNpc(cls);
            return MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(o.ptr());
        };

        npc["getBaseDisposition"] = [](const Object& o, const Object& player) -> int {
            const MWWorld::Class& cls = o.ptr().getClass();
            verifyPlayer(player);
            verifyNpc(cls);
            return cls.getNpcStats(o.ptr()).getBaseDisposition();
        };

        npc["setBaseDisposition"] = [](Object& o, const Object& player, int value) {
            if (dynamic_cast<LObject*>(&o) && !dynamic_cast<SelfObject*>(&o))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Class& cls = o.ptr().getClass();
            verifyPlayer(player);
            verifyNpc(cls);
            cls.getNpcStats(o.ptr()).setBaseDisposition(value);
        };

        npc["modifyBaseDisposition"] = [](Object& o, const Object& player, int value) {
            if (dynamic_cast<LObject*>(&o) && !dynamic_cast<SelfObject*>(&o))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Class& cls = o.ptr().getClass();
            verifyPlayer(player);
            verifyNpc(cls);
            auto& stats = cls.getNpcStats(o.ptr());
            stats.setBaseDisposition(stats.getBaseDisposition() + value);
        };

        npc["createRecordDraft"] = tableToNPC;
        npc["getFactionRank"] = [](const Object& actor, std::string_view faction) -> size_t {
            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            const MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);
            if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            {
                if (npcStats.isInFaction(factionId))
                {
                    int factionRank = npcStats.getFactionRank(factionId);
                    return LuaUtil::toLuaIndex(factionRank);
                }
            }
            else
            {
                ESM::RefId primaryFactionId = ptr.getClass().getPrimaryFaction(ptr);
                if (factionId == primaryFactionId)
                    return LuaUtil::toLuaIndex(ptr.getClass().getPrimaryFactionRank(ptr));
            }
            return 0;
        };

        npc["setFactionRank"] = [](Object& actor, std::string_view faction, int value) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            const ESM::Faction* factionPtr
                = MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionId);

            auto ranksCount = static_cast<int>(getValidRanksCount(factionPtr));
            if (value <= 0 || value > ranksCount)
                throw std::runtime_error("Requested rank does not exist");

            auto targetRank = LuaUtil::fromLuaIndex(std::clamp(value, 1, ranksCount));

            if (ptr != MWBase::Environment::get().getWorld()->getPlayerPtr())
            {
                ESM::RefId primaryFactionId = ptr.getClass().getPrimaryFaction(ptr);
                if (factionId != primaryFactionId)
                    throw std::runtime_error("Only players can modify ranks in non-primary factions");
            }

            MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);
            if (!npcStats.isInFaction(factionId))
                throw std::runtime_error("Target actor is not a member of faction " + factionId.toDebugString());

            npcStats.setFactionRank(factionId, targetRank);
        };

        npc["modifyFactionRank"] = [](Object& actor, std::string_view faction, int value) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            if (value == 0)
                return;

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            const ESM::Faction* factionPtr
                = MWBase::Environment::get().getESMStore()->get<ESM::Faction>().search(factionId);
            if (!factionPtr)
                return;

            auto ranksCount = static_cast<int>(getValidRanksCount(factionPtr));

            MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);

            if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            {
                int currentRank = npcStats.getFactionRank(factionId);
                if (currentRank >= 0)
                    npcStats.setFactionRank(factionId, std::clamp(currentRank + value, 0, ranksCount - 1));
                else
                    throw std::runtime_error("Target actor is not a member of faction " + factionId.toDebugString());

                return;
            }

            ESM::RefId primaryFactionId = ptr.getClass().getPrimaryFaction(ptr);
            if (factionId != primaryFactionId)
                throw std::runtime_error("Only players can modify ranks in non-primary factions");

            // If we already changed rank for this NPC, modify current rank in the NPC stats.
            // Otherwise take rank from base NPC record, adjust it and put it to NPC data.
            int currentRank = npcStats.getFactionRank(factionId);
            if (currentRank < 0)
            {
                currentRank = ptr.getClass().getPrimaryFactionRank(ptr);
                npcStats.joinFaction(factionId);
            }

            npcStats.setFactionRank(factionId, std::clamp(currentRank + value, 0, ranksCount - 1));
        };

        npc["joinFaction"] = [](Object& actor, std::string_view faction) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            {
                MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);
                int currentRank = npcStats.getFactionRank(factionId);
                if (currentRank < 0)
                    npcStats.joinFaction(factionId);
                return;
            }

            throw std::runtime_error("Only player can join factions");
        };

        npc["leaveFaction"] = [](Object& actor, std::string_view faction) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            {
                ptr.getClass().getNpcStats(ptr).setFactionRank(factionId, -1);
                return;
            }

            throw std::runtime_error("Only player can leave factions");
        };

        npc["getFactionReputation"] = [](const Object& actor, std::string_view faction) {
            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            return ptr.getClass().getNpcStats(ptr).getFactionReputation(factionId);
        };

        npc["setFactionReputation"] = [](Object& actor, std::string_view faction, int value) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            ptr.getClass().getNpcStats(ptr).setFactionReputation(factionId, value);
        };

        npc["modifyFactionReputation"] = [](Object& actor, std::string_view faction, int value) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);

            MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);
            int existingReputation = npcStats.getFactionReputation(factionId);
            npcStats.setFactionReputation(factionId, existingReputation + value);
        };

        npc["expel"] = [](Object& actor, std::string_view faction) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);
            ptr.getClass().getNpcStats(ptr).expell(factionId, false);
        };
        npc["clearExpelled"] = [](Object& actor, std::string_view faction) {
            if (dynamic_cast<LObject*>(&actor) && !dynamic_cast<SelfObject*>(&actor))
                throw std::runtime_error("Local scripts can modify only self");

            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);
            ptr.getClass().getNpcStats(ptr).clearExpelled(factionId);
        };
        npc["isExpelled"] = [](const Object& actor, std::string_view faction) {
            const MWWorld::Ptr ptr = actor.ptr();
            ESM::RefId factionId = parseFactionId(faction);
            return ptr.getClass().getNpcStats(ptr).getExpelled(factionId);
        };
        npc["getFactions"] = [](sol::this_state lua, const Object& actor) {
            const MWWorld::Ptr ptr = actor.ptr();
            MWMechanics::NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);
            sol::table res(lua, sol::create);
            if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            {
                for (const auto& [factionId, _] : npcStats.getFactionRanks())
                    res.add(factionId.serializeText());
                return res;
            }

            ESM::RefId primaryFactionId = ptr.getClass().getPrimaryFaction(ptr);
            if (primaryFactionId.empty())
                return res;

            res.add(primaryFactionId.serializeText());

            return res;
        };
    }
}

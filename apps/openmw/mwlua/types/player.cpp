#include "types.hpp"

#include <apps/openmw/mwbase/dialoguemanager.hpp>
#include <components/esm3/loadbsgn.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/misc/strings/format.hpp>

#include "../birthsignbindings.hpp"
#include "../luamanagerimp.hpp"

#include "apps/openmw/mwbase/inputmanager.hpp"
#include "apps/openmw/mwbase/journal.hpp"
#include "apps/openmw/mwbase/mechanicsmanager.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwmechanics/npcstats.hpp"
#include "apps/openmw/mwworld/class.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwworld/globals.hpp"
#include "apps/openmw/mwworld/player.hpp"

namespace MWLua
{
    struct Quests
    {
        bool mMutable = false;
    };
    struct Quest
    {
        ESM::RefId mQuestId;
        bool mMutable = false;
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::Quests> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::Quest> : std::false_type
    {
    };
}

namespace
{
    ESM::RefId toBirthSignId(const sol::object& recordOrId)
    {
        if (recordOrId.is<ESM::BirthSign>())
            return recordOrId.as<const ESM::BirthSign*>()->mId;
        std::string_view textId = LuaUtil::cast<std::string_view>(recordOrId);
        ESM::RefId id = ESM::RefId::deserializeText(textId);
        if (!MWBase::Environment::get().getESMStore()->get<ESM::BirthSign>().search(id))
            throw std::runtime_error("Failed to find birth sign: " + std::string(textId));
        return id;
    }

    ESM::RefId parseFactionId(std::string_view faction)
    {
        ESM::RefId id = ESM::RefId::deserializeText(faction);
        if (!MWBase::Environment::get().getESMStore()->get<ESM::Faction>().search(id))
            return ESM::RefId();
        return id;
    }
}

namespace MWLua
{
    static void verifyPlayer(const Object& player)
    {
        if (player.ptr() != MWBase::Environment::get().getWorld()->getPlayerPtr())
            throw std::runtime_error("The argument must be a player!");
    }

    static void verifyNpc(const MWWorld::Class& cls)
    {
        if (!cls.isNpc())
            throw std::runtime_error("The argument must be a NPC!");
    }

    void addPlayerBindings(sol::table player, const Context& context)
    {
        MWBase::Journal* const journal = MWBase::Environment::get().getJournal();

        player["quests"] = [](const Object& player) {
            verifyPlayer(player);
            bool allowChanges = dynamic_cast<const GObject*>(&player) != nullptr
                || dynamic_cast<const SelfObject*>(&player) != nullptr;
            return Quests{ .mMutable = allowChanges };
        };
        sol::state_view lua = context.sol();
        sol::usertype<Quests> quests = lua.new_usertype<Quests>("Quests");
        quests[sol::meta_function::to_string] = [](const Quests& quests) { return "Quests"; };
        quests[sol::meta_function::index] = [](const Quests& quests, std::string_view questId) -> sol::optional<Quest> {
            ESM::RefId quest = ESM::RefId::deserializeText(questId);
            const ESM::Dialogue* dial = MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>().search(quest);
            if (dial == nullptr || dial->mType != ESM::Dialogue::Journal)
                return sol::nullopt;
            return Quest{ .mQuestId = quest, .mMutable = quests.mMutable };
        };
        quests[sol::meta_function::pairs] = [journal](const Quests& quests) {
            std::vector<ESM::RefId> ids;
            for (auto it = journal->questBegin(); it != journal->questEnd(); ++it)
                ids.push_back(it->first);
            size_t i = 0;
            return [ids = std::move(ids), i,
                       allowChanges = quests.mMutable]() mutable -> sol::optional<std::tuple<std::string, Quest>> {
                if (i >= ids.size())
                    return sol::nullopt;
                const ESM::RefId& id = ids[i++];
                return std::make_tuple(id.serializeText(), Quest{ .mQuestId = id, .mMutable = allowChanges });
            };
        };

        sol::usertype<Quest> quest = lua.new_usertype<Quest>("Quest");
        quest[sol::meta_function::to_string]
            = [](const Quest& quest) { return "Quest[" + quest.mQuestId.serializeText() + "]"; };

        auto getQuestStage = [journal](const Quest& q) -> int {
            const MWDialogue::Quest* quest = journal->getQuestOrNull(q.mQuestId);
            if (quest == nullptr)
                return 0;
            return journal->getJournalIndex(q.mQuestId);
        };
        auto setQuestStage = [context](const Quest& q, int stage) {
            if (!q.mMutable)
                throw std::runtime_error("Value can only be changed in global or player scripts!");
            context.mLuaManager->addAction(
                [q, stage] { MWBase::Environment::get().getJournal()->setJournalIndex(q.mQuestId, stage); },
                "setQuestStageAction");
        };
        quest["stage"] = sol::property(getQuestStage, setQuestStage);

        quest["id"] = sol::readonly_property([](const Quest& q) -> std::string { return q.mQuestId.serializeText(); });
        quest["started"] = sol::readonly_property(
            [journal](const Quest& q) { return journal->getQuestOrNull(q.mQuestId) != nullptr; });
        quest["finished"] = sol::property(
            [journal](const Quest& q) -> bool {
                const MWDialogue::Quest* quest = journal->getQuestOrNull(q.mQuestId);
                if (quest == nullptr)
                    return false;
                return quest->isFinished();
            },
            [journal, context](const Quest& q, bool finished) {
                if (!q.mMutable)
                    throw std::runtime_error("Value can only be changed in global or player scripts!");
                context.mLuaManager->addAction(
                    [q, finished, journal] { journal->getOrStartQuest(q.mQuestId).setFinished(finished); },
                    "setQuestFinishedAction");
            });
        quest["addJournalEntry"] = [context](const Quest& q, int stage, sol::optional<GObject> actor) {
            if (!q.mMutable)
                throw std::runtime_error("Can only be used in global or player scripts!");
            // The journal mwscript function has a try function here, we will make the lua function throw an
            // error. However, the addAction will cause it to error outside of this function.
            context.mLuaManager->addAction(
                [actor = std::move(actor), q, stage] {
                    MWWorld::Ptr actorPtr;
                    if (actor)
                        actorPtr = actor->ptr();
                    MWBase::Environment::get().getJournal()->addEntry(q.mQuestId, stage, actorPtr);
                },
                "addJournalEntryAction");
        };

        player["CONTROL_SWITCH"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, std::string_view>(lua,
                {
                    { "Controls", "playercontrols" },
                    { "Fighting", "playerfighting" },
                    { "Jumping", "playerjumping" },
                    { "Looking", "playerlooking" },
                    { "Magic", "playermagic" },
                    { "ViewMode", "playerviewswitch" },
                    { "VanityMode", "vanitymode" },
                }));

        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        player["getControlSwitch"] = [input](const Object& player, std::string_view key) {
            verifyPlayer(player);
            return input->getControlSwitch(key);
        };
        player["setControlSwitch"] = [input](const Object& player, std::string_view key, bool v) {
            verifyPlayer(player);
            if (dynamic_cast<const LObject*>(&player) && !dynamic_cast<const SelfObject*>(&player))
                throw std::runtime_error("Only player and global scripts can toggle control switches.");
            input->toggleControlSwitch(key, v);
        };
        player["isTeleportingEnabled"] = [](const Object& player) -> bool {
            verifyPlayer(player);
            return MWBase::Environment::get().getWorld()->isTeleportingEnabled();
        };
        player["setTeleportingEnabled"] = [](const Object& player, bool state) {
            verifyPlayer(player);
            if (dynamic_cast<const LObject*>(&player) && !dynamic_cast<const SelfObject*>(&player))
                throw std::runtime_error("Only player and global scripts can toggle teleportation.");
            MWBase::Environment::get().getWorld()->enableTeleporting(state);
        };
        player["addTopic"] = [](const Object& player, std::string_view topicId) {
            verifyPlayer(player);

            ESM::RefId topic = ESM::RefId::deserializeText(topicId);
            const ESM::Dialogue* dialogueRecord
                = MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>().search(topic);

            if (!dialogueRecord)
                throw std::runtime_error(
                    Misc::StringUtils::format("Failed to add topic ", topicId, ": topic record not found"));
            else if (dialogueRecord->mType != ESM::Dialogue::Topic)
                throw std::runtime_error(
                    Misc::StringUtils::format("Failed to add topic ", topicId, ": record is not a topic"));

            MWBase::Environment::get().getDialogueManager()->addTopic(topic);
        };
        player["sendMenuEvent"] = [context](const Object& player, std::string eventName, const sol::object& eventData) {
            verifyPlayer(player);
            context.mLuaEvents->addMenuEvent({ std::move(eventName), LuaUtil::serialize(eventData) });
        };

        player["getCrimeLevel"] = [](const Object& o) -> int {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getNpcStats(o.ptr()).getBounty();
        };
        player["setCrimeLevel"] = [](const Object& o, int amount) {
            verifyPlayer(o);
            if (!dynamic_cast<const GObject*>(&o))
                throw std::runtime_error("Only global scripts can change crime level");
            const MWWorld::Class& cls = o.ptr().getClass();
            cls.getNpcStats(o.ptr()).setBounty(amount);
            if (amount == 0)
                MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();
        };
        player["isCharGenFinished"] = [](const Object&) -> bool {
            return MWBase::Environment::get().getWorld()->getGlobalFloat(MWWorld::Globals::sCharGenState) == -1;
        };

        player["OFFENSE_TYPE"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(context.sol(),
                { { "Theft", MWBase::MechanicsManager::OffenseType::OT_Theft },
                    { "Assault", MWBase::MechanicsManager::OffenseType::OT_Assault },
                    { "Murder", MWBase::MechanicsManager::OffenseType::OT_Murder },
                    { "Trespassing", MWBase::MechanicsManager::OffenseType::OT_Trespassing },
                    { "SleepingInOwnedBed", MWBase::MechanicsManager::OffenseType::OT_SleepingInOwnedBed },
                    { "Pickpocket", MWBase::MechanicsManager::OffenseType::OT_Pickpocket } }));
        player["_runStandardCommitCrime"] = [](const Object& o, const sol::optional<Object> victim, int type,
                                                std::string_view faction, int arg = 0, bool victimAware = false) {
            verifyPlayer(o);
            if (victim.has_value() && !victim->ptrOrEmpty().isEmpty())
                verifyNpc(victim->ptrOrEmpty().getClass());
            if (!dynamic_cast<const GObject*>(&o))
                throw std::runtime_error("Only global scripts can commit crime");
            if (type < 0 || type > MWBase::MechanicsManager::OffenseType::OT_Pickpocket)
                throw std::runtime_error("Invalid offense type");

            ESM::RefId factionId = parseFactionId(faction);
            // If the faction is provided but not found, error out
            if (faction != "" && factionId == ESM::RefId())
                throw std::runtime_error("Faction does not exist");

            MWWorld::Ptr victimObj = nullptr;
            if (victim.has_value())
                victimObj = victim->ptrOrEmpty();
            return MWBase::Environment::get().getMechanicsManager()->commitCrime(o.ptr(), victimObj,
                static_cast<MWBase::MechanicsManager::OffenseType>(type), factionId, arg, victimAware);
        };

        player["birthSigns"] = initBirthSignRecordBindings(context);
        player["getBirthSign"] = [](const Object& player) -> std::string {
            verifyPlayer(player);
            return MWBase::Environment::get().getWorld()->getPlayer().getBirthSign().serializeText();
        };
        player["setBirthSign"] = [](const Object& player, const sol::object& recordOrId) {
            verifyPlayer(player);
            if (!dynamic_cast<const GObject*>(&player))
                throw std::runtime_error("Only global scripts can change birth signs");
            MWBase::Environment::get().getWorld()->getPlayer().setBirthSign(toBirthSignId(recordOrId));
        };
    }
}

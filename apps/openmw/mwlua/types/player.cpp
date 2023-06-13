#include "types.hpp"

#include "../luamanagerimp.hpp"
#include <apps/openmw/mwbase/journal.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwmechanics/npcstats.hpp>
#include <apps/openmw/mwworld/class.hpp>

namespace MWLua
{
    struct Quests
    {
        bool mMutable = false;
        MWWorld::SafePtr::Id playerId;
        using Iterator = typename MWBase::Journal::TQuestIter;
        Iterator mIterator;
        MWBase::Journal* const journal = MWBase::Environment::get().getJournal();
        void reset() { mIterator = journal->questBegin(); }
        bool isEnd() const { return mIterator == journal->questEnd(); }
        void advance() { mIterator++; }
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

namespace MWLua
{

    void addPlayerQuestBindings(sol::table& player, const Context& context)
    {
        MWBase::Journal* const journal = MWBase::Environment::get().getJournal();

        // Quests
        player["quests"] = [](const Object& player) {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            Quests q = {};
            if (player.ptr() != world->getPlayerPtr())
                throw std::runtime_error("Must provide a player!");
            if (dynamic_cast<const GObject*>(&player))
                q.mMutable = true;
            q.playerId = player.id();
            return q;
        };
        sol::usertype<Quests> quests = context.mLua->sol().new_usertype<Quests>("Quests");
        quests[sol::meta_function::to_string]
            = [](const Quests& quests) { return "Quests[" + quests.playerId.toString() + "]"; };
        quests[sol::meta_function::length] = [journal]() { return journal->getQuestCount(); };
        quests[sol::meta_function::index] = sol::overload([](const Quests& quests, std::string_view index) -> Quest {
            Quest q;
            q.mQuestId = ESM::RefId::deserializeText(index);
            q.mMutable = quests.mMutable;
            return q;
        });
        quests[sol::meta_function::pairs] = [](sol::this_state ts, Quests& self) {
            sol::state_view lua(ts);
            self.reset();
            return sol::as_function([lua, &self]() mutable -> std::pair<sol::object, sol::object> {
                if (!self.isEnd())
                {
                    Quest q;
                    q.mQuestId = (self.mIterator->first);
                    q.mMutable = self.mMutable;
                    auto result = sol::make_object(lua, q);
                    auto index = sol::make_object(lua, self.mIterator->first);
                    self.advance();
                    return { index, result };
                }
                else
                {
                    return { sol::lua_nil, sol::lua_nil };
                }
            });
        };

        // Quest Functions
        auto getQuestStage = [journal](const Quest& q) -> int {
            auto quest = journal->getQuestPtr(q.mQuestId);
            if (quest == nullptr)
                return -1;
            return journal->getJournalIndex(q.mQuestId);
        };
        auto setQuestStage = [context](const Quest& q, int stage) {
            if (!q.mMutable)
                throw std::runtime_error("Value can only be changed in global scripts!");
            context.mLuaManager->addAction(
                [q, stage] { MWBase::Environment::get().getJournal()->setJournalIndex(q.mQuestId, stage); },
                "setQuestStageAction");
        };

        // Player quests
        sol::usertype<Quest> quest = context.mLua->sol().new_usertype<Quest>("Quest");
        quest[sol::meta_function::to_string]
            = [](const Quest& quest) { return "Quest [" + quest.mQuestId.serializeText() + "]"; };
        quest["stage"] = sol::property(getQuestStage, setQuestStage);
        quest["name"] = sol::readonly_property([journal](const Quest& q) -> sol::optional<std::string_view> {
            auto quest = journal->getQuestPtr(q.mQuestId);
            if (quest == nullptr)
                return sol::nullopt;
            return quest->getName();
        });
        quest["id"] = sol::readonly_property([](const Quest& q) -> std::string { return q.mQuestId.serializeText(); });
        quest["isFinished"] = sol::property(
            [journal](const Quest& q) -> bool {
                auto quest = journal->getQuestPtr(q.mQuestId);
                if (quest == nullptr)
                    return false;
                return quest->isFinished();
            },
            [journal, context](const Quest& q, bool finished) {
                if (!q.mMutable)
                    throw std::runtime_error("Value can only be changed in global scripts!");
                context.mLuaManager->addAction(
                    [q, finished, journal] { journal->getQuest(q.mQuestId).setFinished(finished); },
                    "setQuestFinishedAction");
            });
        quest["addJournalEntry"] = [context](const Quest& q, const GObject& actor, int stage) {
            MWWorld::Ptr ptr = actor.ptr();

            // The journal mwscript function has a try function here, we will make the lua function throw an
            // error. However, the addAction will cause it to error outside of this function.
            context.mLuaManager->addAction(
                [ptr, q, stage] { MWBase::Environment::get().getJournal()->addEntry(q.mQuestId, stage, ptr); },
                "addJournalEntryAction");
        };
    }

    void addPlayerBindings(sol::table player, const Context& context)
    {
        player["getCrimeLevel"] = [](const Object& o) -> int {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getNpcStats(o.ptr()).getBounty();
        };
        addPlayerQuestBindings(player, context);
    }
}
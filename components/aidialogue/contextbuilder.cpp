#include "contextbuilder.hpp"

#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadinfo.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/journal.hpp>
#include <apps/openmw/mwbase/world.hpp>

#include <apps/openmw/mwmechanics/creaturestats.hpp>
#include <apps/openmw/mwmechanics/npcstats.hpp>

#include <apps/openmw/mwworld/class.hpp>
#include <apps/openmw/mwworld/containerstore.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>
#include <apps/openmw/mwworld/ptr.hpp>

#include <sstream>

namespace AIDialogue
{
    struct ContextBuilder::Impl
    {
        std::vector<std::pair<std::string, std::string>> conversationHistory;
        int maxJournalEntries = 20;
        int maxHistoryExchanges = 5;
    };

    ContextBuilder::ContextBuilder()
        : mImpl(std::make_unique<Impl>())
    {
    }

    ContextBuilder::~ContextBuilder() = default;

    void ContextBuilder::addExchange(const std::string& playerInput, const std::string& npcResponse)
    {
        mImpl->conversationHistory.emplace_back(playerInput, npcResponse);

        // Keep only recent exchanges to manage token limits
        if (mImpl->conversationHistory.size() > static_cast<size_t>(mImpl->maxHistoryExchanges))
        {
            mImpl->conversationHistory.erase(mImpl->conversationHistory.begin());
        }
    }

    void ContextBuilder::clearHistory()
    {
        mImpl->conversationHistory.clear();
    }

    void ContextBuilder::setMaxJournalEntries(int max)
    {
        mImpl->maxJournalEntries = max;
    }

    DialogueContext ContextBuilder::buildContext(
        const MWWorld::Ptr& actor, const ESM::RefId& topic, const MWBase::Journal& journal)
    {
        DialogueContext context;

        // Extract NPC information
        context.npcName = actor.getClass().getName(actor);

        if (actor.get<ESM::NPC>()->mBase)
        {
            const ESM::NPC* npc = actor.get<ESM::NPC>()->mBase;

            // Get race
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Race* race = store.get<ESM::Race>().search(npc->mRace);
            if (race)
                context.npcRace = race->mName;

            // Get class
            const ESM::Class* cls = store.get<ESM::Class>().search(npc->mClass);
            if (cls)
                context.npcClass = cls->mName;

            // Get gender
            context.npcGender = npc->isMale() ? "male" : "female";

            // Get faction
            if (!npc->mFaction.empty())
            {
                const ESM::Faction* faction = store.get<ESM::Faction>().search(npc->mFaction);
                if (faction)
                    context.npcFaction = faction->mName;
            }
        }

        // Get NPC disposition
        if (actor.getClass().isNpc())
        {
            context.disposition = static_cast<int>(actor.getClass().getCreatureStats(actor).getAiSetting(
                MWMechanics::AiSetting::Hello).getModified());
        }

        // Extract player information
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        context.playerName = player.getClass().getName(player);
        context.playerLevel = player.getClass().getCreatureStats(player).getLevel();

        if (player.get<ESM::NPC>()->mBase)
        {
            const ESM::NPC* playerNpc = player.get<ESM::NPC>()->mBase;
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            const ESM::Race* playerRace = store.get<ESM::Race>().search(playerNpc->mRace);
            if (playerRace)
                context.playerRace = playerRace->mName;

            const ESM::Class* playerClass = store.get<ESM::Class>().search(playerNpc->mClass);
            if (playerClass)
                context.playerClass = playerClass->mName;
        }

        // Get player skills (top skills only)
        if (player.getClass().isNpc())
        {
            const MWMechanics::NpcStats& stats = player.getClass().getNpcStats(player);
            // Get a few notable skills for context
            context.playerSkills["combat"] = static_cast<int>(stats.getSkill(ESM::Skill::LongBlade).getModified());
            context.playerSkills["magic"] = static_cast<int>(stats.getSkill(ESM::Skill::Destruction).getModified());
            context.playerSkills["stealth"] = static_cast<int>(stats.getSkill(ESM::Skill::Sneak).getModified());
        }

        // Get notable items (weapons, quest items, etc.)
        // For now, just count gold
        MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
        int gold = store.count(ESM::RefId::stringRefId("gold_001"));
        if (gold > 0)
        {
            context.notableItems.push_back("Gold: " + std::to_string(gold));
        }

        // Get relevant journal entries
        context.relevantJournalEntries = getRelevantJournalEntries(topic, context.npcFaction, journal);

        // Set current topic
        context.currentTopic = topic.toDebugString();

        // Add conversation history
        context.recentExchanges = mImpl->conversationHistory;

        // Get location
        context.location = player.getCell()->getCell()->getDescription();

        // Get special states
        const MWMechanics::NpcStats& playerStats = player.getClass().getNpcStats(player);
        context.playerIsVampire = playerStats.isVampire();
        context.playerIsWerewolf = playerStats.isWerewolf();

        return context;
    }

    std::vector<DialogueContext::JournalEntryInfo> ContextBuilder::getRelevantJournalEntries(
        const ESM::RefId& topic, const std::string& npcFaction, const MWBase::Journal& journal)
    {
        std::vector<DialogueContext::JournalEntryInfo> entries;

        // Get all quests
        for (auto questIter = journal.questBegin(); questIter != journal.questEnd(); ++questIter)
        {
            const MWDialogue::Quest& quest = questIter->second;
            ESM::RefId questId = questIter->first;

            DialogueContext::JournalEntryInfo info;
            info.questId = questId.toDebugString();
            info.questIndex = quest.getIndex();
            info.isFinished = quest.isFinished();
            info.isActive = !quest.isFinished();

            // Get the most recent entry for this quest
            for (auto entryIter = quest.begin(); entryIter != quest.end(); ++entryIter)
            {
                info.text = entryIter->getText();
            }

            if (!info.text.empty())
            {
                entries.push_back(info);
            }

            // Limit entries to manage token count
            if (entries.size() >= static_cast<size_t>(mImpl->maxJournalEntries))
                break;
        }

        return entries;
    }

    std::string ContextBuilder::getNPCPersonality(const DialogueContext& context)
    {
        std::ostringstream personality;

        personality << "You are " << context.npcName;

        if (!context.npcRace.empty())
            personality << ", a " << context.npcRace;

        if (!context.npcClass.empty())
            personality << " " << context.npcClass;

        personality << ".";

        if (!context.npcFaction.empty())
        {
            personality << " You are a member of " << context.npcFaction << ".";
        }

        // Add disposition-based personality hints
        if (context.disposition < 30)
        {
            personality << " You are hostile and distrustful of strangers.";
        }
        else if (context.disposition < 50)
        {
            personality << " You are cautious and reserved.";
        }
        else if (context.disposition < 70)
        {
            personality << " You are friendly but professional.";
        }
        else
        {
            personality << " You are warm and helpful.";
        }

        return personality.str();
    }

    std::string ContextBuilder::formatAsSystemPrompt(const DialogueContext& context)
    {
        std::ostringstream prompt;

        // Core personality and role
        prompt << getNPCPersonality(context) << "\n\n";

        // World context
        prompt << "SETTING:\n";
        prompt << "- Location: " << context.location << "\n";
        if (!context.weather.empty())
            prompt << "- Weather: " << context.weather << "\n";
        if (!context.timeOfDay.empty())
            prompt << "- Time: " << context.timeOfDay << "\n";
        prompt << "\n";

        // Player information
        prompt << "ABOUT THE PLAYER:\n";
        prompt << "- Name: " << context.playerName << "\n";
        if (!context.playerRace.empty())
            prompt << "- Race: " << context.playerRace << "\n";
        if (!context.playerClass.empty())
            prompt << "- Class: " << context.playerClass << "\n";
        prompt << "- Level: " << context.playerLevel << "\n";

        if (context.playerIsVampire)
            prompt << "- Status: Vampire\n";
        if (context.playerIsWerewolf)
            prompt << "- Status: Werewolf\n";
        if (context.playerBounty > 0)
            prompt << "- Bounty: " << context.playerBounty << " gold\n";

        prompt << "\n";

        // Journal context (KEY FEATURE!)
        if (!context.relevantJournalEntries.empty())
        {
            prompt << "QUEST KNOWLEDGE (what you know about ongoing events):\n";
            for (const auto& entry : context.relevantJournalEntries)
            {
                prompt << "- " << entry.questId << " (Stage " << entry.questIndex << "): ";
                prompt << entry.text << "\n";
            }
            prompt << "\n";
        }

        // Conversation history
        if (!context.recentExchanges.empty())
        {
            prompt << "CONVERSATION HISTORY:\n";
            for (const auto& exchange : context.recentExchanges)
            {
                prompt << "Player: " << exchange.first << "\n";
                prompt << "You: " << exchange.second << "\n";
            }
            prompt << "\n";
        }

        // Current topic
        if (!context.currentTopic.empty())
        {
            prompt << "CURRENT TOPIC: " << context.currentTopic << "\n\n";
        }

        // Instructions for AI behavior
        prompt << "INSTRUCTIONS:\n";
        prompt << "1. Stay in character as " << context.npcName << "\n";
        prompt << "2. Respond naturally and concisely (2-4 sentences)\n";
        prompt << "3. Reference quest knowledge when relevant\n";
        prompt << "4. Maintain appropriate tone based on disposition (" << context.disposition << "/100)\n";
        prompt << "5. If you need to update game state, use commands in this format:\n";
        prompt << "   [COMMAND:ADD_JOURNAL:quest_id:index:text]\n";
        prompt << "   [COMMAND:ADD_TOPIC:topic_id]\n";
        prompt << "   [COMMAND:MODIFY_DISPOSITION:amount]\n";
        prompt << "6. Provide your response as JSON: {\"dialogue\": \"your response\", \"commands\": []}\n";

        return prompt.str();
    }
}

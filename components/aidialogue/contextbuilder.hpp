#ifndef COMPONENTS_AIDIALOGUE_CONTEXTBUILDER_HPP
#define COMPONENTS_AIDIALOGUE_CONTEXTBUILDER_HPP

#include <components/esm/refid.hpp>

#include <map>
#include <string>
#include <vector>

namespace MWWorld
{
    class Ptr;
}

namespace MWBase
{
    class Journal;
}

namespace ESM
{
    struct DialInfo;
}

namespace AIDialogue
{
    /// \brief Context information for AI dialogue generation
    struct DialogueContext
    {
        // NPC Information
        std::string npcName;
        std::string npcRace;
        std::string npcClass;
        std::string npcFaction;
        std::string npcGender; // "male" or "female"
        int disposition = 50;

        // Player State
        int playerLevel = 1;
        std::string playerName;
        std::string playerRace;
        std::string playerClass;
        std::map<std::string, int> playerSkills;
        std::vector<std::string> notableItems;

        // Journal Context (KEY FEATURE - incremental knowledge)
        struct JournalEntryInfo
        {
            std::string questId;
            int questIndex;
            std::string text;
            bool isActive;
            bool isFinished;
        };
        std::vector<JournalEntryInfo> relevantJournalEntries;

        // Conversation State
        std::string currentTopic;
        std::vector<std::pair<std::string, std::string>> recentExchanges; // (player, npc)

        // World Context
        std::string location;
        std::string weather;
        std::string timeOfDay;

        // Special States
        bool playerIsVampire = false;
        bool playerIsWerewolf = false;
        int playerBounty = 0;
    };

    /// \brief Builds AI context from game state
    class ContextBuilder
    {
    public:
        ContextBuilder();
        ~ContextBuilder();

        /// Build complete context for a dialogue with an NPC
        /// \param actor The NPC being talked to
        /// \param topic Current topic being discussed
        /// \param journal Journal system for quest context
        /// \return Complete context for AI prompt
        DialogueContext buildContext(
            const MWWorld::Ptr& actor, const ESM::RefId& topic, const MWBase::Journal& journal);

        /// Format context as a system prompt for the AI
        /// \param context Dialogue context
        /// \return Formatted prompt string
        std::string formatAsSystemPrompt(const DialogueContext& context);

        /// Add a conversational exchange to context
        void addExchange(const std::string& playerInput, const std::string& npcResponse);

        /// Clear conversation history
        void clearHistory();

        /// Set maximum number of journal entries to include (to manage token limits)
        void setMaxJournalEntries(int max);

    private:
        struct Impl;
        std::unique_ptr<Impl> mImpl;

        std::string getNPCPersonality(const DialogueContext& context);
        std::vector<DialogueContext::JournalEntryInfo> getRelevantJournalEntries(
            const ESM::RefId& topic, const std::string& npcFaction, const MWBase::Journal& journal);
    };
}

#endif // COMPONENTS_AIDIALOGUE_CONTEXTBUILDER_HPP

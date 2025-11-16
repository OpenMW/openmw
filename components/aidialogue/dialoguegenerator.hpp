#ifndef COMPONENTS_AIDIALOGUE_DIALOGUEGENERATOR_HPP
#define COMPONENTS_AIDIALOGUE_DIALOGUEGENERATOR_HPP

#include "contextbuilder.hpp"
#include "openrouterclient.hpp"

#include <components/esm/refid.hpp>

#include <memory>
#include <string>
#include <vector>

namespace AIDialogue
{
    /// \brief Commands that the AI can issue to modify game state
    struct GameCommand
    {
        enum class Type
        {
            AddJournal, // Add journal entry
            SetQuestIndex, // Update quest progress
            AddTopic, // Make topic known to player
            ModifyDisposition, // Change NPC disposition
            GiveItem, // Give item to player
            TakeItem, // Remove item from player
            SetGlobal, // Set global variable
            StartCombat, // Initiate combat
            Unknown
        };

        Type type = Type::Unknown;
        std::map<std::string, std::string> parameters;
    };

    /// \brief Complete AI response with dialogue and commands
    struct AIResponse
    {
        bool success = false;
        std::string dialogueText;
        std::vector<GameCommand> commands;
        std::vector<std::string> newTopics;
        std::string errorMessage;
        int tokensUsed = 0;
    };

    /// \brief Orchestrates AI dialogue generation
    class DialogueGenerator
    {
    public:
        DialogueGenerator();
        ~DialogueGenerator();

        /// Initialize with API client
        void initialize(std::shared_ptr<OpenRouterClient> client);

        /// Generate AI response to player input
        /// \param context Complete game context
        /// \param playerInput What the player said/asked
        /// \param config Generation parameters
        /// \return AI response with dialogue and commands
        AIResponse generateResponse(
            const DialogueContext& context, const std::string& playerInput, const GenerationConfig& config);

        /// Generate greeting dialogue (when conversation starts)
        /// \param context Complete game context
        /// \param config Generation parameters
        /// \return AI-generated greeting
        AIResponse generateGreeting(const DialogueContext& context, const GenerationConfig& config);

        /// Check if generator is ready to use
        bool isReady() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> mImpl;

        std::string buildPrompt(const DialogueContext& context, const std::string& userInput);
        AIResponse parseAIOutput(const std::string& aiOutput);
        std::vector<GameCommand> extractCommands(const std::string& text);
        GameCommand parseCommand(const std::string& commandLine);
    };

    /// \brief Utility for parsing command syntax
    /// Commands use format: [COMMAND:TYPE:param1:param2:...]
    namespace CommandParser
    {
        std::vector<std::string> findCommandBlocks(const std::string& text);
        GameCommand parseCommandBlock(const std::string& block);
        std::string stripCommands(const std::string& text);
    }
}

#endif // COMPONENTS_AIDIALOGUE_DIALOGUEGENERATOR_HPP

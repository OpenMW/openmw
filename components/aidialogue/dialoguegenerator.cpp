#include "dialoguegenerator.hpp"

#include "json.hpp"

#include <algorithm>
#include <regex>
#include <sstream>

namespace AIDialogue
{
    struct DialogueGenerator::Impl
    {
        std::shared_ptr<OpenRouterClient> client;
        ContextBuilder contextBuilder;
    };

    DialogueGenerator::DialogueGenerator()
        : mImpl(std::make_unique<Impl>())
    {
    }

    DialogueGenerator::~DialogueGenerator() = default;

    void DialogueGenerator::initialize(std::shared_ptr<OpenRouterClient> client)
    {
        mImpl->client = client;
    }

    bool DialogueGenerator::isReady() const
    {
        return mImpl->client && mImpl->client->isConfigured();
    }

    std::string DialogueGenerator::buildPrompt(const DialogueContext& context, const std::string& userInput)
    {
        std::ostringstream prompt;
        prompt << mImpl->contextBuilder.formatAsSystemPrompt(context);
        prompt << "\nPlayer says: \"" << userInput << "\"\n\n";
        prompt << "Respond as " << context.npcName << " in JSON format.";
        return prompt.str();
    }

    AIResponse DialogueGenerator::generateResponse(
        const DialogueContext& context, const std::string& playerInput, const GenerationConfig& config)
    {
        AIResponse response;

        if (!isReady())
        {
            response.errorMessage = "DialogueGenerator not initialized";
            return response;
        }

        // Build messages for API call
        std::vector<Message> messages;

        // System prompt with full context
        messages.emplace_back(Message::Role::System, mImpl->contextBuilder.formatAsSystemPrompt(context));

        // User message
        messages.emplace_back(Message::Role::User, playerInput);

        // Call API
        APIResponse apiResponse = mImpl->client->chatCompletion(messages, config);

        if (!apiResponse.success)
        {
            response.errorMessage = apiResponse.errorMessage;
            return response;
        }

        response.tokensUsed = apiResponse.totalTokens;

        // Parse AI output
        response = parseAIOutput(apiResponse.content);
        response.tokensUsed = apiResponse.totalTokens;

        return response;
    }

    AIResponse DialogueGenerator::generateGreeting(const DialogueContext& context, const GenerationConfig& config)
    {
        return generateResponse(context, "Hello", config);
    }

    AIResponse DialogueGenerator::parseAIOutput(const std::string& aiOutput)
    {
        AIResponse response;

        // Try to parse as JSON first
        JSONParser parser(aiOutput);

        if (parser.isValid() && parser.hasKey("dialogue"))
        {
            // JSON format
            response.dialogueText = parser.getString("dialogue");

            // Extract commands if present
            if (parser.hasKey("commands"))
            {
                JSONParser commandsArray = parser.getObject("commands");
                int numCommands = commandsArray.getArraySize();

                for (int i = 0; i < numCommands; i++)
                {
                    JSONParser cmdObj = commandsArray.getArrayElement(i);
                    GameCommand cmd;

                    std::string typeStr = cmdObj.getString("type");
                    if (typeStr == "ADD_JOURNAL")
                        cmd.type = GameCommand::Type::AddJournal;
                    else if (typeStr == "SET_QUEST_INDEX")
                        cmd.type = GameCommand::Type::SetQuestIndex;
                    else if (typeStr == "ADD_TOPIC")
                        cmd.type = GameCommand::Type::AddTopic;
                    else if (typeStr == "MODIFY_DISPOSITION")
                        cmd.type = GameCommand::Type::ModifyDisposition;
                    else
                        continue;

                    // Extract parameters (implementation depends on command type)
                    response.commands.push_back(cmd);
                }
            }
        }
        else
        {
            // Fallback: parse inline commands
            response.commands = extractCommands(aiOutput);
            response.dialogueText = CommandParser::stripCommands(aiOutput);
        }

        response.success = !response.dialogueText.empty();
        return response;
    }

    std::vector<GameCommand> DialogueGenerator::extractCommands(const std::string& text)
    {
        std::vector<GameCommand> commands;
        std::vector<std::string> commandBlocks = CommandParser::findCommandBlocks(text);

        for (const auto& block : commandBlocks)
        {
            GameCommand cmd = CommandParser::parseCommandBlock(block);
            if (cmd.type != GameCommand::Type::Unknown)
            {
                commands.push_back(cmd);
            }
        }

        return commands;
    }

    // CommandParser namespace implementation

    namespace CommandParser
    {
        std::vector<std::string> findCommandBlocks(const std::string& text)
        {
            std::vector<std::string> blocks;
            std::regex commandRegex(R"(\[COMMAND:[^\]]+\])");

            auto begin = std::sregex_iterator(text.begin(), text.end(), commandRegex);
            auto end = std::sregex_iterator();

            for (std::sregex_iterator i = begin; i != end; ++i)
            {
                blocks.push_back(i->str());
            }

            return blocks;
        }

        GameCommand parseCommandBlock(const std::string& block)
        {
            GameCommand cmd;

            // Remove [COMMAND: and ]
            if (block.size() < 10 || block.substr(0, 9) != "[COMMAND:")
                return cmd;

            std::string content = block.substr(9, block.size() - 10);

            // Split by colons
            std::vector<std::string> parts;
            std::istringstream stream(content);
            std::string part;

            while (std::getline(stream, part, ':'))
            {
                parts.push_back(part);
            }

            if (parts.empty())
                return cmd;

            // Parse command type
            std::string typeStr = parts[0];
            if (typeStr == "ADD_JOURNAL")
            {
                cmd.type = GameCommand::Type::AddJournal;
                if (parts.size() >= 4)
                {
                    cmd.parameters["quest_id"] = parts[1];
                    cmd.parameters["index"] = parts[2];
                    cmd.parameters["text"] = parts[3];
                }
            }
            else if (typeStr == "SET_QUEST_INDEX")
            {
                cmd.type = GameCommand::Type::SetQuestIndex;
                if (parts.size() >= 3)
                {
                    cmd.parameters["quest_id"] = parts[1];
                    cmd.parameters["index"] = parts[2];
                }
            }
            else if (typeStr == "ADD_TOPIC")
            {
                cmd.type = GameCommand::Type::AddTopic;
                if (parts.size() >= 2)
                {
                    cmd.parameters["topic_id"] = parts[1];
                }
            }
            else if (typeStr == "MODIFY_DISPOSITION")
            {
                cmd.type = GameCommand::Type::ModifyDisposition;
                if (parts.size() >= 2)
                {
                    cmd.parameters["amount"] = parts[1];
                }
            }
            else if (typeStr == "GIVE_ITEM")
            {
                cmd.type = GameCommand::Type::GiveItem;
                if (parts.size() >= 3)
                {
                    cmd.parameters["item_id"] = parts[1];
                    cmd.parameters["count"] = parts[2];
                }
            }
            else if (typeStr == "SET_GLOBAL")
            {
                cmd.type = GameCommand::Type::SetGlobal;
                if (parts.size() >= 3)
                {
                    cmd.parameters["variable"] = parts[1];
                    cmd.parameters["value"] = parts[2];
                }
            }

            return cmd;
        }

        std::string stripCommands(const std::string& text)
        {
            std::regex commandRegex(R"(\[COMMAND:[^\]]+\])");
            return std::regex_replace(text, commandRegex, "");
        }
    }
}

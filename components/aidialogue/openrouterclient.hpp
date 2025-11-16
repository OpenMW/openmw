#ifndef COMPONENTS_AIDIALOGUE_OPENROUTERCLIENT_HPP
#define COMPONENTS_AIDIALOGUE_OPENROUTERCLIENT_HPP

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace AIDialogue
{
    /// \brief Message in a conversation
    struct Message
    {
        enum class Role
        {
            System,
            User,
            Assistant
        };

        Role role;
        std::string content;

        Message(Role r, std::string c)
            : role(r)
            , content(std::move(c))
        {
        }
    };

    /// \brief Configuration for AI generation
    struct GenerationConfig
    {
        std::string model = "meta-llama/llama-3.1-8b-instruct:free"; // Default to free model
        float temperature = 0.7f;
        int maxTokens = 500;
        bool jsonMode = true; // Request JSON structured output
        float topP = 0.9f;
        float frequencyPenalty = 0.0f;
        float presencePenalty = 0.0f;
    };

    /// \brief Response from OpenRouter API
    struct APIResponse
    {
        bool success = false;
        std::string content;
        std::string errorMessage;
        int promptTokens = 0;
        int completionTokens = 0;
        int totalTokens = 0;
        std::string modelUsed;
    };

    /// \brief HTTP client for OpenRouter API
    /// Uses libcurl for HTTP requests with proper error handling
    class OpenRouterClient
    {
    public:
        OpenRouterClient();
        ~OpenRouterClient();

        /// Set the API key (from settings)
        void setApiKey(const std::string& apiKey);

        /// Set optional headers for attribution
        void setAppInfo(const std::string& appName, const std::string& siteUrl);

        /// Make a synchronous chat completion request
        /// \param messages Conversation history
        /// \param config Generation parameters
        /// \return API response with generated content
        APIResponse chatCompletion(const std::vector<Message>& messages, const GenerationConfig& config);

        /// Check if client is properly configured
        bool isConfigured() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> mImpl;
    };

    /// \brief Utility functions for JSON conversion
    namespace JSON
    {
        std::string messagesToJson(const std::vector<Message>& messages);
        std::string configToJson(const GenerationConfig& config);
        std::optional<APIResponse> parseResponse(const std::string& jsonResponse);
    }
}

#endif // COMPONENTS_AIDIALOGUE_OPENROUTERCLIENT_HPP

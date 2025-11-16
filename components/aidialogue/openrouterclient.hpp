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

    /// \brief Provider routing configuration for OpenRouter
    struct ProviderRouting
    {
        std::vector<std::string> order; // Try providers in this order
        bool allowFallbacks = true; // Allow OpenRouter's default fallbacks
        std::vector<std::string> only; // Only use these providers (whitelist)
        std::vector<std::string> ignore; // Never use these providers (blacklist)
        std::string sort; // "price", "throughput", or "latency"
        bool requireParameters = false; // Ensure provider supports all parameters
        bool zeroDataRetention = false; // Enforce ZDR endpoints
        std::vector<std::string> quantizations; // Filter by quantization (e.g., "int4", "int8")

        /// Helper: Create routing for Cerebras-first with fallbacks
        static ProviderRouting cerebrasFirst()
        {
            ProviderRouting routing;
            routing.order = { "Cerebras", "Groq", "Together" };
            routing.allowFallbacks = true;
            return routing;
        }

        /// Helper: Cerebras-only (no fallbacks)
        static ProviderRouting cerebrasOnly()
        {
            ProviderRouting routing;
            routing.only = { "Cerebras" };
            routing.allowFallbacks = false;
            return routing;
        }

        /// Helper: Parse from comma-separated string
        static ProviderRouting fromString(const std::string& providers, bool allowFallbacks = true)
        {
            ProviderRouting routing;
            routing.allowFallbacks = allowFallbacks;

            size_t start = 0;
            size_t end = providers.find(',');

            while (end != std::string::npos)
            {
                routing.order.push_back(providers.substr(start, end - start));
                start = end + 1;
                end = providers.find(',', start);
            }

            if (start < providers.length())
            {
                routing.order.push_back(providers.substr(start));
            }

            return routing;
        }
    };

    /// \brief Configuration for AI generation
    struct GenerationConfig
    {
        // Model selection
        std::string model = "meta-llama/llama-4-maverick:free"; // Cerebras-optimized default

        // Provider routing
        ProviderRouting providerRouting;

        // Generation parameters
        float temperature = 0.7f;
        int maxTokens = 500;
        float topP = 0.9f;
        float frequencyPenalty = 0.3f;
        float presencePenalty = 0.2f;

        // Output format
        bool jsonMode = true; // Request JSON structured output

        // Advanced
        int seed = -1; // -1 = random, otherwise deterministic
        std::vector<std::string> stopSequences;

        /// Helper: Create config optimized for Cerebras dialogue
        static GenerationConfig cerebrasDialogue()
        {
            GenerationConfig config;
            config.model = "meta-llama/llama-4-maverick:free";
            config.providerRouting = ProviderRouting::cerebrasFirst();
            config.temperature = 0.7f;
            config.maxTokens = 500;
            config.topP = 0.9f;
            config.frequencyPenalty = 0.3f;
            config.presencePenalty = 0.2f;
            config.jsonMode = true;
            return config;
        }

        /// Helper: Create config for summarization
        static GenerationConfig cerebrasSummary()
        {
            GenerationConfig config;
            config.model = "qwen/qwen3-235b-a22b-thinking-2507";
            config.providerRouting = ProviderRouting::cerebrasFirst();
            config.temperature = 0.1f; // More deterministic for summaries
            config.maxTokens = 2048;
            config.jsonMode = false;
            return config;
        }
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
        std::string providerUsed; // Which provider actually handled the request
        float costUSD = 0.0f; // Estimated cost in USD
    };

    /// \brief HTTP client for OpenRouter API with Cerebras optimization
    /// Uses libcurl for HTTP requests with proper error handling and retries
    class OpenRouterClient
    {
    public:
        OpenRouterClient();
        ~OpenRouterClient();

        /// Set the API key (from settings or environment)
        void setApiKey(const std::string& apiKey);

        /// Set optional headers for attribution (improves OpenRouter rankings)
        void setAppInfo(const std::string& appName, const std::string& siteUrl);

        /// Set retry configuration
        void setRetryConfig(int maxRetries, int retryDelayMs);

        /// Set request timeout
        void setTimeout(int timeoutMs);

        /// Make a synchronous chat completion request
        /// \param messages Conversation history
        /// \param config Generation parameters with provider routing
        /// \return API response with generated content
        APIResponse chatCompletion(const std::vector<Message>& messages, const GenerationConfig& config);

        /// Make request with automatic retry on failure
        /// \param messages Conversation history
        /// \param config Generation parameters
        /// \param maxRetries Override default retry count
        /// \return API response
        APIResponse chatCompletionWithRetry(
            const std::vector<Message>& messages, const GenerationConfig& config, int maxRetries = -1);

        /// Check if client is properly configured
        bool isConfigured() const;

        /// Get last request cost
        float getLastCost() const;

        /// Get cumulative session cost
        float getSessionCost() const;

        /// Reset session cost tracker
        void resetSessionCost();

    private:
        struct Impl;
        std::unique_ptr<Impl> mImpl;

        std::string buildRequestBody(const std::vector<Message>& messages, const GenerationConfig& config);
        APIResponse performRequest(const std::string& requestBody);
    };

    /// \brief Utility functions for JSON conversion
    namespace JSON
    {
        std::string messagesToJson(const std::vector<Message>& messages);
        std::string configToJson(const GenerationConfig& config);
        std::string providerRoutingToJson(const ProviderRouting& routing);
        std::optional<APIResponse> parseResponse(const std::string& jsonResponse);
    }
}

#endif // COMPONENTS_AIDIALOGUE_OPENROUTERCLIENT_HPP

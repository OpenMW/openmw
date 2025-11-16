#include "openrouterclient.hpp"

#include "json.hpp"

#include <curl/curl.h>

#include <cstring>
#include <sstream>
#include <thread>

namespace AIDialogue
{
    // Callback for curl to write response data
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
    {
        size_t totalSize = size * nmemb;
        userp->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    struct OpenRouterClient::Impl
    {
        std::string apiKey;
        std::string appName = "OpenMW AI Dialogue";
        std::string siteUrl = "https://openmw.org";
        CURL* curl = nullptr;
        int timeoutMs = 30000;
        int maxRetries = 3;
        int retryDelayMs = 1000;
        float lastCost = 0.0f;
        float sessionCost = 0.0f;

        Impl() { curl = curl_easy_init(); }

        ~Impl()
        {
            if (curl)
                curl_easy_cleanup(curl);
        }
    };

    OpenRouterClient::OpenRouterClient()
        : mImpl(std::make_unique<Impl>())
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    OpenRouterClient::~OpenRouterClient()
    {
        curl_global_cleanup();
    }

    void OpenRouterClient::setApiKey(const std::string& apiKey)
    {
        mImpl->apiKey = apiKey;
    }

    void OpenRouterClient::setAppInfo(const std::string& appName, const std::string& siteUrl)
    {
        mImpl->appName = appName;
        mImpl->siteUrl = siteUrl;
    }

    void OpenRouterClient::setRetryConfig(int maxRetries, int retryDelayMs)
    {
        mImpl->maxRetries = maxRetries;
        mImpl->retryDelayMs = retryDelayMs;
    }

    void OpenRouterClient::setTimeout(int timeoutMs)
    {
        mImpl->timeoutMs = timeoutMs;
    }

    bool OpenRouterClient::isConfigured() const
    {
        return !mImpl->apiKey.empty() && mImpl->curl != nullptr;
    }

    float OpenRouterClient::getLastCost() const
    {
        return mImpl->lastCost;
    }

    float OpenRouterClient::getSessionCost() const
    {
        return mImpl->sessionCost;
    }

    void OpenRouterClient::resetSessionCost()
    {
        mImpl->sessionCost = 0.0f;
    }

    std::string OpenRouterClient::buildRequestBody(const std::vector<Message>& messages, const GenerationConfig& config)
    {
        JSONBuilder requestBuilder;
        requestBuilder.startObject();
        requestBuilder.addString("model", config.model);

        // Add messages array
        requestBuilder.startArray("messages");
        for (const auto& msg : messages)
        {
            requestBuilder.startObject();
            std::string roleStr;
            switch (msg.role)
            {
                case Message::Role::System:
                    roleStr = "system";
                    break;
                case Message::Role::User:
                    roleStr = "user";
                    break;
                case Message::Role::Assistant:
                    roleStr = "assistant";
                    break;
            }
            requestBuilder.addString("role", roleStr);
            requestBuilder.addString("content", msg.content);
            requestBuilder.endObject();
        }
        requestBuilder.endArray();

        // Add generation parameters
        requestBuilder.addFloat("temperature", config.temperature);
        requestBuilder.addInt("max_tokens", config.maxTokens);
        requestBuilder.addFloat("top_p", config.topP);
        requestBuilder.addFloat("frequency_penalty", config.frequencyPenalty);
        requestBuilder.addFloat("presence_penalty", config.presencePenalty);

        // Add provider routing (KEY FEATURE!)
        const auto& routing = config.providerRouting;
        if (!routing.order.empty() || !routing.only.empty() || !routing.ignore.empty())
        {
            requestBuilder.addRawValue("provider", JSON::providerRoutingToJson(routing));
        }

        // Add JSON mode if requested
        if (config.jsonMode)
        {
            requestBuilder.addRawValue("response_format", "{\"type\":\"json_object\"}");
        }

        // Add seed if specified
        if (config.seed >= 0)
        {
            requestBuilder.addInt("seed", config.seed);
        }

        // Add stop sequences if any
        if (!config.stopSequences.empty())
        {
            requestBuilder.startArray("stop");
            for (const auto& seq : config.stopSequences)
            {
                requestBuilder.startObject();
                requestBuilder.addString("sequence", seq);
                requestBuilder.endObject();
            }
            requestBuilder.endArray();
        }

        requestBuilder.endObject();
        return requestBuilder.build();
    }

    APIResponse OpenRouterClient::performRequest(const std::string& requestBody)
    {
        APIResponse response;
        std::string responseData;
        struct curl_slist* headers = nullptr;

        // Set headers
        std::string authHeader = "Authorization: Bearer " + mImpl->apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        if (!mImpl->siteUrl.empty())
        {
            std::string refererHeader = "HTTP-Referer: " + mImpl->siteUrl;
            headers = curl_slist_append(headers, refererHeader.c_str());
        }

        if (!mImpl->appName.empty())
        {
            std::string titleHeader = "X-Title: " + mImpl->appName;
            headers = curl_slist_append(headers, titleHeader.c_str());
        }

        // Configure curl
        curl_easy_setopt(mImpl->curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
        curl_easy_setopt(mImpl->curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(mImpl->curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(mImpl->curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(mImpl->curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(mImpl->curl, CURLOPT_TIMEOUT_MS, static_cast<long>(mImpl->timeoutMs));

        // Perform request
        CURLcode res = curl_easy_perform(mImpl->curl);

        // Cleanup
        curl_slist_free_all(headers);

        if (res != CURLE_OK)
        {
            response.errorMessage = std::string("CURL error: ") + curl_easy_strerror(res);
            return response;
        }

        // Check HTTP status code
        long httpCode = 0;
        curl_easy_getinfo(mImpl->curl, CURLINFO_RESPONSE_CODE, &httpCode);

        if (httpCode != 200)
        {
            response.errorMessage = "HTTP error: " + std::to_string(httpCode);
            return response;
        }

        // Parse response
        try
        {
            JSONParser parser(responseData);

            if (!parser.isValid())
            {
                response.errorMessage = "Invalid JSON response";
                return response;
            }

            // Check for API errors
            if (parser.hasKey("error"))
            {
                JSONParser errorObj = parser.getObject("error");
                response.errorMessage = errorObj.getString("message", "Unknown API error");
                return response;
            }

            // Extract choices array
            JSONParser choicesArray = parser.getObject("choices");
            if (choicesArray.isValid() && choicesArray.getArraySize() > 0)
            {
                JSONParser firstChoice = choicesArray.getArrayElement(0);
                JSONParser message = firstChoice.getObject("message");
                response.content = message.getString("content");
            }

            // Extract usage info
            if (parser.hasKey("usage"))
            {
                JSONParser usage = parser.getObject("usage");
                response.promptTokens = usage.getInt("prompt_tokens", 0);
                response.completionTokens = usage.getInt("completion_tokens", 0);
                response.totalTokens = usage.getInt("total_tokens", 0);
            }

            // Extract model used
            response.modelUsed = parser.getString("model", "");

            // Extract provider used (OpenRouter-specific field)
            if (parser.hasKey("provider"))
            {
                response.providerUsed = parser.getString("provider", "unknown");
            }

            response.success = !response.content.empty();

            // Track cost (rough estimate)
            // Free models cost $0, paid models vary widely
            mImpl->lastCost = 0.0f; // TODO: Implement actual cost tracking
            mImpl->sessionCost += mImpl->lastCost;
        }
        catch (const std::exception& e)
        {
            response.errorMessage = std::string("Error parsing response: ") + e.what();
            return response;
        }

        return response;
    }

    APIResponse OpenRouterClient::chatCompletion(const std::vector<Message>& messages, const GenerationConfig& config)
    {
        if (!isConfigured())
        {
            APIResponse response;
            response.errorMessage = "OpenRouterClient not configured (missing API key)";
            return response;
        }

        std::string requestBody = buildRequestBody(messages, config);
        return performRequest(requestBody);
    }

    APIResponse OpenRouterClient::chatCompletionWithRetry(
        const std::vector<Message>& messages, const GenerationConfig& config, int maxRetries)
    {
        int retries = maxRetries >= 0 ? maxRetries : mImpl->maxRetries;
        int currentDelay = mImpl->retryDelayMs;

        APIResponse lastResponse;

        for (int attempt = 0; attempt <= retries; attempt++)
        {
            lastResponse = chatCompletion(messages, config);

            if (lastResponse.success)
            {
                return lastResponse;
            }

            // Don't retry on certain errors (like invalid API key)
            if (lastResponse.errorMessage.find("401") != std::string::npos
                || lastResponse.errorMessage.find("Invalid API key") != std::string::npos)
            {
                return lastResponse;
            }

            // Wait before retrying (exponential backoff)
            if (attempt < retries)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(currentDelay));
                currentDelay *= 2; // Exponential backoff
            }
        }

        return lastResponse;
    }

    // JSON namespace utility functions
    namespace JSON
    {
        std::string providerRoutingToJson(const ProviderRouting& routing)
        {
            JSONBuilder builder;
            builder.startObject();

            // Add order array if specified
            if (!routing.order.empty())
            {
                builder.startArray("order");
                for (const auto& provider : routing.order)
                {
                    builder.startObject();
                    builder.addString("provider", provider);
                    builder.endObject();
                }
                builder.endArray();
            }

            // Add allow_fallbacks
            builder.addBool("allow_fallbacks", routing.allowFallbacks);

            // Add only array if specified
            if (!routing.only.empty())
            {
                builder.startArray("only");
                for (const auto& provider : routing.only)
                {
                    builder.addString("", provider);
                }
                builder.endArray();
            }

            // Add ignore array if specified
            if (!routing.ignore.empty())
            {
                builder.startArray("ignore");
                for (const auto& provider : routing.ignore)
                {
                    builder.addString("", provider);
                }
                builder.endArray();
            }

            // Add sort if specified
            if (!routing.sort.empty())
            {
                builder.addString("sort", routing.sort);
            }

            // Add require_parameters
            if (routing.requireParameters)
            {
                builder.addBool("require_parameters", true);
            }

            // Add zdr (Zero Data Retention)
            if (routing.zeroDataRetention)
            {
                builder.addBool("zdr", true);
            }

            // Add quantizations if specified
            if (!routing.quantizations.empty())
            {
                builder.startArray("quantizations");
                for (const auto& quant : routing.quantizations)
                {
                    builder.addString("", quant);
                }
                builder.endArray();
            }

            builder.endObject();
            return builder.build();
        }

        std::string messagesToJson(const std::vector<Message>& messages)
        {
            JSONBuilder builder;
            builder.startArray("messages");
            for (const auto& msg : messages)
            {
                builder.startObject();
                std::string roleStr;
                switch (msg.role)
                {
                    case Message::Role::System:
                        roleStr = "system";
                        break;
                    case Message::Role::User:
                        roleStr = "user";
                        break;
                    case Message::Role::Assistant:
                        roleStr = "assistant";
                        break;
                }
                builder.addString("role", roleStr);
                builder.addString("content", msg.content);
                builder.endObject();
            }
            builder.endArray();
            return builder.build();
        }

        std::string configToJson(const GenerationConfig& config)
        {
            JSONBuilder builder;
            builder.startObject();
            builder.addString("model", config.model);
            builder.addFloat("temperature", config.temperature);
            builder.addInt("max_tokens", config.maxTokens);
            builder.addFloat("top_p", config.topP);
            builder.addFloat("frequency_penalty", config.frequencyPenalty);
            builder.addFloat("presence_penalty", config.presencePenalty);

            if (!config.providerRouting.order.empty())
            {
                builder.addRawValue("provider", providerRoutingToJson(config.providerRouting));
            }

            if (config.jsonMode)
            {
                builder.addRawValue("response_format", "{\"type\":\"json_object\"}");
            }

            builder.endObject();
            return builder.build();
        }

        std::optional<APIResponse> parseResponse(const std::string& jsonResponse)
        {
            JSONParser parser(jsonResponse);

            if (!parser.isValid())
                return std::nullopt;

            APIResponse response;

            if (parser.hasKey("error"))
            {
                JSONParser errorObj = parser.getObject("error");
                response.errorMessage = errorObj.getString("message");
                return response;
            }

            JSONParser choicesArray = parser.getObject("choices");
            if (choicesArray.isValid() && choicesArray.getArraySize() > 0)
            {
                JSONParser firstChoice = choicesArray.getArrayElement(0);
                JSONParser message = firstChoice.getObject("message");
                response.content = message.getString("content");
            }

            if (parser.hasKey("usage"))
            {
                JSONParser usage = parser.getObject("usage");
                response.promptTokens = usage.getInt("prompt_tokens");
                response.completionTokens = usage.getInt("completion_tokens");
                response.totalTokens = usage.getInt("total_tokens");
            }

            response.modelUsed = parser.getString("model");
            response.providerUsed = parser.getString("provider", "unknown");
            response.success = !response.content.empty();

            return response;
        }
    }
}

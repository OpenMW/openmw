#include "openrouterclient.hpp"

#include "json.hpp"

#include <curl/curl.h>

#include <cstring>
#include <sstream>

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
        std::string appName = "OpenMW";
        std::string siteUrl = "https://openmw.org";
        CURL* curl = nullptr;

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

    bool OpenRouterClient::isConfigured() const
    {
        return !mImpl->apiKey.empty() && mImpl->curl != nullptr;
    }

    APIResponse OpenRouterClient::chatCompletion(const std::vector<Message>& messages, const GenerationConfig& config)
    {
        APIResponse response;

        if (!isConfigured())
        {
            response.errorMessage = "OpenRouterClient not configured (missing API key)";
            return response;
        }

        // Build request JSON
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

        // Add JSON mode if requested
        if (config.jsonMode)
        {
            requestBuilder.addRawValue("response_format", "{\"type\":\"json_object\"}");
        }

        requestBuilder.endObject();

        std::string requestBody = requestBuilder.build();

        // Prepare curl request
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
        curl_easy_setopt(mImpl->curl, CURLOPT_TIMEOUT, 30L); // 30 second timeout

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

            response.modelUsed = parser.getString("model", config.model);
            response.success = !response.content.empty();
        }
        catch (const std::exception& e)
        {
            response.errorMessage = std::string("Error parsing response: ") + e.what();
            return response;
        }

        return response;
    }

    // JSON namespace utility functions
    namespace JSON
    {
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
            response.success = !response.content.empty();

            return response;
        }
    }
}

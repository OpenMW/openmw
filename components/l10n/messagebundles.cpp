#include "messagebundles.hpp"

#include <charconv>
#include <cstring>
#include <mutex>
#include <optional>
#include <span>

#include <unicode/calendar.h>
#include <unicode/errorcode.h>
#include <yaml-cpp/yaml.h>

#include <components/debug/debuglog.hpp>
#include <components/misc/messageformatparser.hpp>

namespace L10n
{
    namespace
    {
        std::string getErrorText(const UParseError& parseError)
        {
            icu::UnicodeString preContext(parseError.preContext), postContext(parseError.postContext);
            std::string parseErrorString;
            preContext.toUTF8String(parseErrorString);
            postContext.toUTF8String(parseErrorString);
            return parseErrorString;
        }

        template <class... Args>
        bool checkSuccess(const icu::ErrorCode& status, const UParseError& parseError, Args const&... message)
        {
            if (status.isFailure())
            {
                std::string errorText = getErrorText(parseError);
                if (!errorText.empty())
                {
                    (Log(Debug::Error) << ... << message)
                        << ": " << status.errorName() << " in \"" << errorText << "\"";
                }
                else
                {
                    (Log(Debug::Error) << ... << message) << ": " << status.errorName();
                }
            }
            return status.isSuccess();
        }

        std::optional<icu::MessageFormat> parseMessageFormat(
            const icu::Locale& lang, std::string_view key, std::string_view value, std::string_view locale)
        {
            icu::UnicodeString pattern
                = icu::UnicodeString::fromUTF8(icu::StringPiece(value.data(), static_cast<std::int32_t>(value.size())));
            icu::ErrorCode status;
            UParseError parseError;
            icu::MessageFormat message(pattern, lang, parseError, status);
            if (checkSuccess(status, parseError, "Failed to create message ", key, " for locale ", locale))
                return message;
            return {};
        }

        template <class T>
        using StringMap = std::unordered_map<std::string, T, Misc::StringUtils::StringHash, std::equal_to<>>;

        void loadLocaleYaml(const YAML::Node& data, const icu::Locale& lang, StringMap<icu::MessageFormat>& bundle)
        {
            const std::string_view localeName = lang.getName();
            for (const auto& it : data)
            {
                const auto key = it.first.as<std::string>();
                const auto value = it.second.as<std::string>();
                std::optional<icu::MessageFormat> message = parseMessageFormat(lang, key, value, localeName);
                if (message)
                    bundle.emplace(key, *message);
            }
        }

        constexpr std::string_view gmstTokenStart = "{gmst:";

        void loadGmstYaml(const YAML::Node& data, StringMap<GmstMessageFormat>& gmsts)
        {
            for (const auto& it : data)
            {
                const auto key = it.first.as<std::string>();
                GmstMessageFormat message;
                if (it.second.IsMap())
                {
                    message.mPattern = it.second["pattern"].as<std::string>();
                    if (YAML::Node variables = it.second["variables"])
                        message.mVariableNames = variables.as<std::vector<std::vector<std::string>>>();
                    message.mReplaceFormat = true;
                }
                else
                {
                    const auto value = it.second.as<std::string>();
                    message.mPattern.reserve(gmstTokenStart.size() + 1 + value.size());
                    message.mPattern = gmstTokenStart;
                    message.mPattern += value;
                    message.mPattern += '}';
                }
                gmsts.emplace(key, std::move(message));
            }
        }

        class GmstFormatParser : public Misc::MessageFormatParser
        {
            std::array<char, 20> mBuffer;
            std::string& mOut;
            std::span<const std::string> mVariableNames;
            std::size_t mVariableIndex;

        public:
            GmstFormatParser(std::string& out, std::span<const std::string> variables)
                : mOut(out)
                , mVariableNames(variables)
                , mVariableIndex(0)
            {
            }

        protected:
            void visitedPlaceholder(Placeholder, int, int, int, Notation) override
            {
                mOut += '{';
                if (mVariableIndex < mVariableNames.size() && !mVariableNames[mVariableIndex].empty())
                    mOut += mVariableNames[mVariableIndex];
                else
                {
                    const auto [ptr, ec]
                        = std::to_chars(mBuffer.data(), mBuffer.data() + mBuffer.size(), mVariableIndex);
                    if (ec == std::errc())
                        mOut += std::string_view(mBuffer.data(), ptr);
                }
                mOut += '}';
                mVariableIndex++;
            }

            void visitedCharacter(char c) override
            {
                if (c == '\'' || c == '{' || c == '}')
                    mOut += '\'';
                mOut += c;
            }
        };

        std::optional<icu::MessageFormat> convertToMessageFormat(
            std::string_view key, const GmstMessageFormat& gmstFormat, const GmstLoader& gmstLoader)
        {
            std::string formatString;
            std::size_t offset = 0;
            std::size_t tokenIndex = 0;
            const std::string_view pattern(gmstFormat.mPattern);
            while (offset < pattern.size())
            {
                const std::size_t start = pattern.find(gmstTokenStart, offset);
                if (start == std::string_view::npos)
                {
                    formatString += pattern.substr(offset);
                    break;
                }
                const std::size_t tokenStart = start + gmstTokenStart.size();
                const std::size_t end = pattern.find_first_of("{}", tokenStart);
                if (end == std::string_view::npos || pattern[end] == '{')
                {
                    // Not a GMST token
                    formatString += pattern.substr(offset, end - offset);
                    offset = end;
                    continue;
                }
                // Replace GMST token
                formatString += pattern.substr(offset, start - offset);
                offset = end + 1;
                std::string_view gmst = pattern.substr(tokenStart, end - tokenStart);
                const std::string* value = gmstLoader(gmst);
                const auto appendEscaped = [&](std::string_view string) {
                    for (char c : string)
                    {
                        if (c == '\'' || c == '{' || c == '}')
                            formatString += '\'';
                        formatString += c;
                    }
                };
                if (value == nullptr)
                {
                    // Unknown GMST string
                    formatString += "GMST:";
                    appendEscaped(gmst);
                }
                else if (gmstFormat.mReplaceFormat)
                {
                    std::span<const std::string> variableNames;
                    if (tokenIndex < gmstFormat.mVariableNames.size())
                        variableNames = gmstFormat.mVariableNames[tokenIndex];
                    GmstFormatParser parser(formatString, variableNames);
                    parser.process(*value);
                }
                else
                    appendEscaped(*value);
                tokenIndex++;
            }
            const icu::Locale& english = icu::Locale::getEnglish();
            return parseMessageFormat(english, key, formatString, "gmst");
        }

        std::string formatArgs(const icu::MessageFormat& message, std::string_view key,
            const std::vector<icu::UnicodeString>& argNames, const std::vector<icu::Formattable>& args)
        {
            icu::UnicodeString result;
            std::string resultString;
            icu::ErrorCode success;
            if (!args.empty() && !argNames.empty())
                message.format(argNames.data(), args.data(), static_cast<std::int32_t>(args.size()), result, success);
            else
                message.format(nullptr, nullptr, static_cast<std::int32_t>(args.size()), result, success);
            checkSuccess(success, {}, "Failed to format message ", key);
            result.toUTF8String(resultString);
            return resultString;
        }
    }

    MessageBundles::MessageBundles(const std::vector<icu::Locale>& preferredLocales, icu::Locale& fallbackLocale)
        : mFallbackLocale(fallbackLocale)
    {
        setPreferredLocales(preferredLocales);
    }

    void MessageBundles::setPreferredLocales(const std::vector<icu::Locale>& preferredLocales)
    {
        mPreferredLocales.clear();
        mPreferredLocaleStrings.clear();
        for (const icu::Locale& loc : preferredLocales)
        {
            mPreferredLocales.push_back(loc);
            mPreferredLocaleStrings.emplace_back(loc.getName());
            // Try without variant or country if they are specified, starting with the most specific
            if (strcmp(loc.getVariant(), "") != 0)
            {
                icu::Locale withoutVariant(loc.getLanguage(), loc.getCountry());
                mPreferredLocales.push_back(withoutVariant);
                mPreferredLocaleStrings.emplace_back(withoutVariant.getName());
            }
            if (strcmp(loc.getCountry(), "") != 0)
            {
                icu::Locale withoutCountry(loc.getLanguage());
                mPreferredLocales.push_back(withoutCountry);
                mPreferredLocaleStrings.emplace_back(withoutCountry.getName());
            }
        }
    }

    void MessageBundles::load(std::istream& input, const icu::Locale& lang)
    {
        YAML::Node data = YAML::Load(input);
        std::string localeName = lang.getName();
        if (localeName == "gmst")
            loadGmstYaml(data, mGmsts);
        else
            loadLocaleYaml(data, lang, mBundles[localeName]);
    }

    const icu::MessageFormat* MessageBundles::findMessage(std::string_view key, std::string_view localeName) const
    {
        std::shared_lock sharedLock(mMutex);
        {
            auto iter = mBundles.find(localeName);
            if (iter != mBundles.end())
            {
                auto message = iter->second.find(key);
                if (message != iter->second.end())
                {
                    return &(message->second);
                }
            }
        }
        if (localeName == "gmst" && mGmstLoader)
        {
            if (!mGmsts.contains(key))
                return nullptr;
            sharedLock.unlock();
            std::unique_lock lock(mMutex);
            auto found = mGmsts.find(key);
            if (found != mGmsts.end())
            {
                auto message = convertToMessageFormat(key, found->second, mGmstLoader);
                mGmsts.erase(found);
                if (message)
                {
                    auto iter = mBundles.find(localeName);
                    if (iter == mBundles.end())
                        iter = mBundles.emplace(localeName, StringMap<icu::MessageFormat>()).first;
                    return &iter->second.emplace(key, *message).first->second;
                }
            }
        }
        return nullptr;
    }

    std::string MessageBundles::formatMessage(
        std::string_view key, const std::map<std::string, icu::Formattable>& args) const
    {
        std::vector<icu::UnicodeString> argNames;
        std::vector<icu::Formattable> argValues;
        for (auto& [k, v] : args)
        {
            argNames.push_back(
                icu::UnicodeString::fromUTF8(icu::StringPiece(k.data(), static_cast<std::int32_t>(k.size()))));
            argValues.push_back(v);
        }
        return formatMessage(key, argNames, argValues);
    }

    std::string MessageBundles::formatMessage(std::string_view key, const std::vector<icu::UnicodeString>& argNames,
        const std::vector<icu::Formattable>& args) const
    {
        for (auto& loc : mPreferredLocaleStrings)
        {
            if (const icu::MessageFormat* message = findMessage(key, loc))
                return formatArgs(*message, key, argNames, args);
        }
        // If no requested locales included the message, try the fallback locale
        if (const icu::MessageFormat* message = findMessage(key, mFallbackLocale.getName()))
            return formatArgs(*message, key, argNames, args);

        icu::Locale defaultLocale(nullptr);
        if (!mPreferredLocales.empty())
        {
            defaultLocale = mPreferredLocales[0];
        }
        std::optional<icu::MessageFormat> defaultMessage = parseMessageFormat(defaultLocale, key, key, "default");
        if (!defaultMessage)
            // If we can't parse the key as a pattern, just return the key
            return std::string(key);
        return formatArgs(*defaultMessage, key, argNames, args);
    }
}

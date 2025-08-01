#include "messagebundles.hpp"

#include <cstring>
#include <unicode/calendar.h>
#include <unicode/errorcode.h>
#include <yaml-cpp/yaml.h>

#include <components/debug/debuglog.hpp>

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

        std::string loadGmst(
            const std::function<std::string(std::string_view)>& gmstLoader, const icu::MessageFormat* message)
        {
            icu::UnicodeString gmstNameUnicode;
            std::string gmstName;
            icu::ErrorCode success;
            message->format(nullptr, nullptr, 0, gmstNameUnicode, success);
            gmstNameUnicode.toUTF8String(gmstName);
            if (gmstLoader)
                return gmstLoader(gmstName);
            return "GMST:" + gmstName;
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
        const icu::Locale& langOrEn = localeName == "gmst" ? icu::Locale::getEnglish() : lang;
        for (const auto& it : data)
        {
            const auto key = it.first.as<std::string>();
            const auto value = it.second.as<std::string>();
            icu::UnicodeString pattern
                = icu::UnicodeString::fromUTF8(icu::StringPiece(value.data(), static_cast<std::int32_t>(value.size())));
            icu::ErrorCode status;
            UParseError parseError;
            icu::MessageFormat message(pattern, langOrEn, parseError, status);
            if (checkSuccess(status, parseError, "Failed to create message ", key, " for locale ", lang.getName()))
            {
                mBundles[localeName].emplace(key, message);
            }
        }
    }

    const icu::MessageFormat* MessageBundles::findMessage(std::string_view key, std::string_view localeName) const
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
        icu::UnicodeString result;
        std::string resultString;
        icu::ErrorCode success;

        const icu::MessageFormat* message = nullptr;
        for (auto& loc : mPreferredLocaleStrings)
        {
            message = findMessage(key, loc);
            if (message)
            {
                if (loc == "gmst")
                    return loadGmst(mGmstLoader, message);
                break;
            }
        }
        // If no requested locales included the message, try the fallback locale
        if (!message)
            message = findMessage(key, mFallbackLocale.getName());

        if (message)
        {
            if (!args.empty() && !argNames.empty())
                message->format(argNames.data(), args.data(), static_cast<std::int32_t>(args.size()), result, success);
            else
                message->format(nullptr, nullptr, static_cast<std::int32_t>(args.size()), result, success);
            checkSuccess(success, {}, "Failed to format message ", key);
            result.toUTF8String(resultString);
            return resultString;
        }
        icu::Locale defaultLocale(nullptr);
        if (!mPreferredLocales.empty())
        {
            defaultLocale = mPreferredLocales[0];
        }
        UParseError parseError;
        icu::MessageFormat defaultMessage(
            icu::UnicodeString::fromUTF8(icu::StringPiece(key.data(), static_cast<std::int32_t>(key.size()))),
            defaultLocale, parseError, success);
        if (!checkSuccess(success, parseError, "Failed to create message ", key))
            // If we can't parse the key as a pattern, just return the key
            return std::string(key);

        if (!args.empty() && !argNames.empty())
            defaultMessage.format(
                argNames.data(), args.data(), static_cast<std::int32_t>(args.size()), result, success);
        else
            defaultMessage.format(nullptr, nullptr, static_cast<std::int32_t>(args.size()), result, success);
        checkSuccess(success, {}, "Failed to format message ", key);
        result.toUTF8String(resultString);
        return resultString;
    }
}

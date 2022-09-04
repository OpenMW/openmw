#include "messagebundles.hpp"

#include <cstring>
#include <unicode/errorcode.h>
#include <unicode/calendar.h>
#include <yaml-cpp/yaml.h>

#include <components/debug/debuglog.hpp>

namespace l10n
{
    MessageBundles::MessageBundles(const std::vector<icu::Locale> &preferredLocales, icu::Locale &fallbackLocale) :
            mFallbackLocale(fallbackLocale)
    {
        setPreferredLocales(preferredLocales);
    }

    void MessageBundles::setPreferredLocales(const std::vector<icu::Locale> &preferredLocales)
    {
        mPreferredLocales.clear();
        mPreferredLocaleStrings.clear();
        for (const icu::Locale &loc: preferredLocales)
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

    std::string getErrorText(const UParseError &parseError)
    {
        icu::UnicodeString preContext(parseError.preContext), postContext(parseError.postContext);
        std::string parseErrorString;
        preContext.toUTF8String(parseErrorString);
        postContext.toUTF8String(parseErrorString);
        return parseErrorString;
    }

    static bool checkSuccess(const icu::ErrorCode &status, const std::string &message, const UParseError parseError = UParseError())
    {
        if (status.isFailure())
        {
            std::string errorText = getErrorText(parseError);
            if (!errorText.empty())
            {
                Log(Debug::Error) << message << ": " << status.errorName() << " in \"" << errorText << "\"";
            }
            else
            {
                Log(Debug::Error) << message << ": " << status.errorName();
            }
        }
        return status.isSuccess();
    }

    void MessageBundles::load(std::istream &input, const icu::Locale& lang, const std::string &path)
    {
        try
        {
            YAML::Node data = YAML::Load(input);
            std::string localeName = lang.getName();
            for (const auto& it: data)
            {
                const auto key = it.first.as<std::string>();
                const auto value = it.second.as<std::string>();
                icu::UnicodeString pattern = icu::UnicodeString::fromUTF8(icu::StringPiece(value.data(), value.size()));
                icu::ErrorCode status;
                UParseError parseError;
                icu::MessageFormat message(pattern, lang, parseError, status);
                if (checkSuccess(status, std::string("Failed to create message ")
                            + key + " for locale " + lang.getName(), parseError))
                {
                    mBundles[localeName].insert(std::make_pair(key, message));
                }
            }
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Can not load " << path << ": " << e.what();
        }
    }

    const icu::MessageFormat * MessageBundles::findMessage(std::string_view key, const std::string &localeName) const
    {
        auto iter = mBundles.find(localeName);
        if (iter != mBundles.end())
        {
            auto message = iter->second.find(key.data());
            if (message != iter->second.end())
            {
                return &(message->second);
            }
        }
        return nullptr;
    }

    std::string MessageBundles::formatMessage(std::string_view key, const std::map<std::string, icu::Formattable> &args) const
    {
        std::vector<icu::UnicodeString> argNames;
        std::vector<icu::Formattable> argValues;
        for (auto& [k, v] : args)
        {
            argNames.push_back(icu::UnicodeString::fromUTF8(icu::StringPiece(k.data(), k.size())));
            argValues.push_back(v);
        }
        return formatMessage(key, argNames, argValues);
    }

    std::string MessageBundles::formatMessage(std::string_view key, const std::vector<icu::UnicodeString> &argNames, const std::vector<icu::Formattable> &args) const
    {
        icu::UnicodeString result;
        std::string resultString;
        icu::ErrorCode success;

        const icu::MessageFormat *message = nullptr;
        for (auto &loc: mPreferredLocaleStrings)
        {
            message = findMessage(key, loc);
            if (message)
                break;
        }
        // If no requested locales included the message, try the fallback locale
        if (!message)
            message = findMessage(key, mFallbackLocale.getName());

        if (message)
        {
            if (!args.empty() && !argNames.empty())
                message->format(argNames.data(), args.data(), args.size(), result, success);
            else
                message->format(nullptr, nullptr, args.size(), result, success);
            checkSuccess(success, std::string("Failed to format message ") + key.data());
            result.toUTF8String(resultString);
            return resultString;
        }
        icu::Locale defaultLocale(nullptr);
        if (!mPreferredLocales.empty())
        {
             defaultLocale = mPreferredLocales[0];
        }
        UParseError parseError;
        icu::MessageFormat defaultMessage(icu::UnicodeString::fromUTF8(icu::StringPiece(key.data(), key.size())),
                                          defaultLocale, parseError, success);
        if (!checkSuccess(success, std::string("Failed to create message ") + key.data(), parseError))
            // If we can't parse the key as a pattern, just return the key
            return std::string(key);

        if (!args.empty() && !argNames.empty())
            defaultMessage.format(argNames.data(), args.data(), args.size(), result, success);
        else
            defaultMessage.format(nullptr, nullptr, args.size(), result, success);
        checkSuccess(success, std::string("Failed to format message ") + key.data());
        result.toUTF8String(resultString);
        return resultString;
    }
}

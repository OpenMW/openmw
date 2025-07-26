#ifndef COMPONENTS_L10N_MESSAGEBUNDLES_H
#define COMPONENTS_L10N_MESSAGEBUNDLES_H

#include <functional>
#include <map>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <unicode/locid.h>
#include <unicode/msgfmt.h>

namespace L10n
{
    /**
     * @brief A collection of Message Bundles
     *
     * Class handling localised message storage and lookup, including fallback locales when messages are missing.
     *
     * If no fallback locale is provided (or a message fails to be found), the key will be formatted instead,
     * or returned verbatim if formatting fails.
     *
     */
    class MessageBundles
    {
    public:
        /* @brief Constructs an empty MessageBundles
         *
         * @param preferredLocales user-requested locales, in order of priority
         *        Each locale will be checked when looking up messages, in case some resource files are incomplete.
         *        For each locale which contains a country code or a variant, the locales obtained by removing first
         *        the variant, then the country code, will also be checked before moving on to the next locale in the
         * list.
         * @param fallbackLocale the fallback locale which should be used if messages cannot be found for the user
         *        preferred locales
         */
        MessageBundles(const std::vector<icu::Locale>& preferredLocales, icu::Locale& fallbackLocale);
        std::string formatMessage(std::string_view key, const std::map<std::string, icu::Formattable>& args) const;
        std::string formatMessage(std::string_view key, const std::vector<icu::UnicodeString>& argNames,
            const std::vector<icu::Formattable>& args) const;
        void setPreferredLocales(const std::vector<icu::Locale>& preferredLocales);
        const std::vector<icu::Locale>& getPreferredLocales() const { return mPreferredLocales; }
        void load(std::istream& input, const icu::Locale& lang);
        bool isLoaded(const icu::Locale& loc) const { return mBundles.find(loc.getName()) != mBundles.end(); }
        const icu::Locale& getFallbackLocale() const { return mFallbackLocale; }
        void setGmstLoader(std::function<std::string(std::string_view)> fn) { mGmstLoader = std::move(fn); }

    private:
        // icu::Locale isn't hashable (or comparable), so we use the string form instead, which is canonicalized
        std::unordered_map<std::string, std::unordered_map<std::string, icu::MessageFormat>> mBundles;
        const icu::Locale mFallbackLocale;
        std::vector<std::string> mPreferredLocaleStrings;
        std::vector<icu::Locale> mPreferredLocales;
        std::function<std::string(std::string_view)> mGmstLoader;
        const icu::MessageFormat* findMessage(std::string_view key, const std::string& localeName) const;
    };

}

#endif // COMPONENTS_L10N_MESSAGEBUNDLES_H

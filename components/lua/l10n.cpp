#include "l10n.hpp"

#include <unicode/errorcode.h>

#include <components/debug/debuglog.hpp>
#include <components/vfs/manager.hpp>

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::L10nManager::Context> : std::false_type {};
}

namespace LuaUtil
{
    void L10nManager::init()
    {
        sol::usertype<Context> ctx = mLua->sol().new_usertype<Context>("L10nContext");
        ctx[sol::meta_function::call] = &Context::translate;
    }

    std::string L10nManager::translate(const std::string& contextName, const std::string& key)
    {
        Context& ctx = getContext(contextName).as<Context>();
        return ctx.translate(key, sol::nil);
    }

    void L10nManager::setPreferredLocales(const std::vector<std::string>& langs)
    {
        mPreferredLocales.clear();
        for (const auto &lang : langs)
            mPreferredLocales.push_back(icu::Locale(lang.c_str()));
        {
            Log msg(Debug::Info);
            msg << "Preferred locales:";
            for (const icu::Locale& l : mPreferredLocales)
                msg << " " << l.getName();
        }
        for (auto& [_, context] : mContexts)
            context.updateLang(this);
    }

    void L10nManager::Context::readLangData(L10nManager* manager, const icu::Locale& lang)
    {
        std::string path = "l10n/";
        path.append(mName);
        path.append("/");
        path.append(lang.getName());
        path.append(".yaml");
        if (!manager->mVFS->exists(path))
            return;

        mMessageBundles->load(*manager->mVFS->get(path), lang, path);
    }

    std::pair<std::vector<icu::Formattable>, std::vector<icu::UnicodeString>> getICUArgs(std::string_view messageId, const sol::table &table)
    {
        std::vector<icu::Formattable> args;
        std::vector<icu::UnicodeString> argNames;
           for (auto elem : table)
        for (auto& [key, value] : table)
        {
            // Argument values
            if (value.is<std::string>())
                args.push_back(icu::Formattable(value.as<std::string>().c_str()));
            // Note: While we pass all numbers as doubles, they still seem to be handled appropriately.
            // Numbers can be forced to be integers using the argType number and argStyle integer
            //     E.g. {var, number, integer}
            else if (value.is<double>())
                args.push_back(icu::Formattable(value.as<double>()));
            else
            {
                Log(Debug::Error) << "Unrecognized argument type for key \"" << key.as<std::string>()
                    << "\" when formatting message \"" << messageId << "\"";
            }

            // Argument names
            const auto str = key.as<std::string>();
            argNames.push_back(icu::UnicodeString::fromUTF8(icu::StringPiece(str.data(), str.size())));
        }
        return std::make_pair(args, argNames);
    }

    std::string L10nManager::Context::translate(std::string_view key, const sol::object& data)
    {
        std::vector<icu::Formattable> args;
        std::vector<icu::UnicodeString> argNames;

        if (data.is<sol::table>()) {
            sol::table dataTable = data.as<sol::table>();
            auto argData = getICUArgs(key, dataTable);
            args = argData.first;
            argNames = argData.second;
        }

        return mMessageBundles->formatMessage(key, argNames, args);
    }

    void L10nManager::Context::updateLang(L10nManager* manager)
    {
        icu::Locale fallbackLocale = mMessageBundles->getFallbackLocale();
        mMessageBundles->setPreferredLocales(manager->mPreferredLocales);
        int localeCount = 0;
        bool fallbackLocaleInPreferred = false;
        for (const icu::Locale& loc: mMessageBundles->getPreferredLocales())
        {
            if (!mMessageBundles->isLoaded(loc))
                readLangData(manager, loc);
            if (mMessageBundles->isLoaded(loc))
            {
                localeCount++;
                Log(Debug::Verbose) << "Language file \"l10n/" << mName << "/" << loc.getName() << ".yaml\" is enabled";
                if (loc == fallbackLocale)
                    fallbackLocaleInPreferred = true;
            }
        }
        if (!mMessageBundles->isLoaded(fallbackLocale))
            readLangData(manager, fallbackLocale);
        if (mMessageBundles->isLoaded(fallbackLocale) && !fallbackLocaleInPreferred)
            Log(Debug::Verbose) << "Fallback language file \"l10n/" << mName << "/" << fallbackLocale.getName() << ".yaml\" is enabled";

        if (localeCount == 0)
        {
            Log(Debug::Warning) << "No language files for the preferred languages found in \"l10n/" << mName << "\"";
        }
    }

    sol::object L10nManager::getContext(const std::string& contextName, const std::string& fallbackLocaleName)
    {
        auto it = mContexts.find(contextName);
        if (it != mContexts.end())
            return sol::make_object(mLua->sol(), it->second);
        auto allowedChar = [](char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') || c == '_';
        };
        bool valid = !contextName.empty();
        for (char c : contextName)
            valid = valid && allowedChar(c);
        if (!valid)
            throw std::runtime_error(std::string("Invalid l10n context name: ") + contextName);
        icu::Locale fallbackLocale(fallbackLocaleName.c_str());
        Context ctx{contextName, std::make_shared<l10n::MessageBundles>(mPreferredLocales, fallbackLocale)};
        {
            Log msg(Debug::Verbose);
            msg << "Fallback locale: " << fallbackLocale.getName();
        }
        ctx.updateLang(this);
        mContexts.emplace(contextName, ctx);
        return sol::make_object(mLua->sol(), ctx);
    }

}

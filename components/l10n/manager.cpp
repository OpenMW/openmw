#include "manager.hpp"

#include <stdexcept>

#include <components/debug/debuglog.hpp>
#include <components/l10n/messagebundles.hpp>
#include <components/vfs/manager.hpp>

namespace l10n
{

    void Manager::setPreferredLocales(const std::vector<std::string>& langs)
    {
        mPreferredLocales.clear();
        for (const auto& lang : langs)
            mPreferredLocales.push_back(icu::Locale(lang.c_str()));
        {
            Log msg(Debug::Info);
            msg << "Preferred locales:";
            for (const icu::Locale& l : mPreferredLocales)
                msg << " " << l.getName();
        }
        for (auto& [key, context] : mCache)
            updateContext(key.first, *context);
    }

    void Manager::readLangData(const std::string& name, MessageBundles& ctx, const icu::Locale& lang)
    {
        std::string path = "l10n/";
        path.append(name);
        path.append("/");
        path.append(lang.getName());
        path.append(".yaml");
        if (!mVFS->exists(path))
            return;

        ctx.load(*mVFS->get(path), lang, path);
    }

    void Manager::updateContext(const std::string& name, MessageBundles& ctx)
    {
        icu::Locale fallbackLocale = ctx.getFallbackLocale();
        ctx.setPreferredLocales(mPreferredLocales);
        int localeCount = 0;
        bool fallbackLocaleInPreferred = false;
        for (const icu::Locale& loc : ctx.getPreferredLocales())
        {
            if (!ctx.isLoaded(loc))
                readLangData(name, ctx, loc);
            if (ctx.isLoaded(loc))
            {
                localeCount++;
                Log(Debug::Verbose) << "Language file \"l10n/" << name << "/" << loc.getName() << ".yaml\" is enabled";
                if (loc == ctx.getFallbackLocale())
                    fallbackLocaleInPreferred = true;
            }
        }
        if (!ctx.isLoaded(ctx.getFallbackLocale()))
            readLangData(name, ctx, ctx.getFallbackLocale());
        if (ctx.isLoaded(ctx.getFallbackLocale()) && !fallbackLocaleInPreferred)
            Log(Debug::Verbose) << "Fallback language file \"l10n/" << name << "/" << ctx.getFallbackLocale().getName()
                                << ".yaml\" is enabled";

        if (localeCount == 0)
        {
            Log(Debug::Warning) << "No language files for the preferred languages found in \"l10n/" << name << "\"";
        }
    }

    std::shared_ptr<const MessageBundles> Manager::getContext(
        const std::string& contextName, const std::string& fallbackLocaleName)
    {
        std::pair<std::string, std::string> key(contextName, fallbackLocaleName);
        auto it = mCache.find(key);
        if (it != mCache.end())
            return it->second;
        auto allowedChar = [](char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        };
        bool valid = !contextName.empty();
        for (char c : contextName)
            valid = valid && allowedChar(c);
        if (!valid)
            throw std::runtime_error(std::string("Invalid l10n context name: ") + contextName);
        icu::Locale fallbackLocale(fallbackLocaleName.c_str());
        std::shared_ptr<MessageBundles> ctx = std::make_shared<MessageBundles>(mPreferredLocales, fallbackLocale);
        {
            Log msg(Debug::Verbose);
            msg << "Fallback locale: " << fallbackLocale.getName();
        }
        updateContext(contextName, *ctx);
        mCache.emplace(key, ctx);
        return ctx;
    }

}

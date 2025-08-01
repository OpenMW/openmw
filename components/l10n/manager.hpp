#ifndef COMPONENTS_L10N_MANAGER_H
#define COMPONENTS_L10N_MANAGER_H

#include <memory>

#include <components/l10n/messagebundles.hpp>

namespace VFS
{
    class Manager;
}

namespace L10n
{

    class Manager
    {
    public:
        Manager(const VFS::Manager* vfs)
            : mVFS(vfs)
        {
        }

        void dropCache() { mCache.clear(); }
        void setPreferredLocales(const std::vector<std::string>& locales, bool gmstHasPriority = true);
        const std::vector<icu::Locale>& getPreferredLocales() const { return mPreferredLocales; }
        void setGmstLoader(std::function<std::string(std::string_view)> fn) { mGmstLoader = std::move(fn); }

        std::shared_ptr<const MessageBundles> getContext(
            std::string_view contextName, const std::string& fallbackLocale = "en");

        std::string getMessage(std::string_view contextName, std::string_view key)
        {
            return getContext(contextName)->formatMessage(key, {}, {});
        }

    private:
        void readLangData(std::string_view name, MessageBundles& ctx, const icu::Locale& lang);
        void updateContext(std::string_view name, MessageBundles& ctx);

        const VFS::Manager* mVFS;
        std::vector<icu::Locale> mPreferredLocales;
        std::map<std::tuple<std::string, std::string>, std::shared_ptr<MessageBundles>, std::less<>> mCache;
        std::function<std::string(std::string_view)> mGmstLoader;
    };

}

#endif // COMPONENTS_L10N_MANAGER_H

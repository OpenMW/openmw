#ifndef COMPONENTS_L10N_MANAGER_H
#define COMPONENTS_L10N_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <unicode/locid.h>

namespace VFS
{
    class Manager;
}

namespace l10n
{
    class MessageBundles;

    class Manager
    {
    public:
        Manager(const VFS::Manager* vfs)
            : mVFS(vfs)
        {
        }

        void dropCache() { mCache.clear(); }
        void setPreferredLocales(const std::vector<std::string>& locales);
        const std::vector<icu::Locale>& getPreferredLocales() const { return mPreferredLocales; }

        std::shared_ptr<const MessageBundles> getContext(
            const std::string& contextName, const std::string& fallbackLocale = "en");

    private:
        void readLangData(const std::string& name, MessageBundles& ctx, const icu::Locale& lang);
        void updateContext(const std::string& name, MessageBundles& ctx);

        const VFS::Manager* mVFS;
        std::vector<icu::Locale> mPreferredLocales;
        std::map<std::pair<std::string, std::string>, std::shared_ptr<MessageBundles>> mCache;
    };

}

#endif // COMPONENTS_L10N_MANAGER_H

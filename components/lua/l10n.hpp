#ifndef COMPONENTS_LUA_I18N_H
#define COMPONENTS_LUA_I18N_H

#include "luastate.hpp"

#include <components/l10n/messagebundles.hpp>

namespace LuaUtil
{

    class L10nManager
    {
    public:
        L10nManager(const VFS::Manager* vfs, LuaState* lua) : mVFS(vfs), mLua(lua) {}
        void init();
        void clear() { mContexts.clear(); }

        void setPreferredLocales(const std::vector<std::string>& locales);
        const std::vector<icu::Locale>& getPreferredLocales() const { return mPreferredLocales; }

        sol::object getContext(const std::string& contextName, const std::string& fallbackLocale = "en");

    private:
        struct Context
        {
            const std::string mName;
            // Must be a shared pointer so that sol::make_object copies the pointer, not the data structure.
            std::shared_ptr<l10n::MessageBundles> mMessageBundles;

            void updateLang(L10nManager* manager);
            void readLangData(L10nManager* manager, const icu::Locale& lang);
            std::string translate(std::string_view key, const sol::object& data);
        };

        const VFS::Manager* mVFS;
        LuaState* mLua;
        std::vector<icu::Locale> mPreferredLocales;
        std::map<std::string, Context> mContexts;
    };

}

#endif // COMPONENTS_LUA_I18N_H

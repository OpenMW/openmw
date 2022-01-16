#ifndef COMPONENTS_LUA_I18N_H
#define COMPONENTS_LUA_I18N_H

#include "luastate.hpp"

namespace LuaUtil
{

    class I18nManager
    {
    public:
        I18nManager(const VFS::Manager* vfs, LuaState* lua) : mVFS(vfs), mLua(lua) {}
        void init();

        void setPreferredLanguages(const std::vector<std::string>& langs);
        const std::vector<std::string>& getPreferredLanguages() const { return mPreferredLanguages; }

        sol::object getContext(const std::string& contextName);

    private:
        struct Context
        {
            std::string mName;
            sol::table mLoadedLangs;
            sol::table mI18n;

            void updateLang(I18nManager* manager);
            void readLangData(I18nManager* manager, const std::string& lang);
            sol::object translate(std::string_view key, const sol::object& data);
        };

        const VFS::Manager* mVFS;
        LuaState* mLua;
        sol::object mI18nLoader = sol::nil;
        std::vector<std::string> mPreferredLanguages;
        std::map<std::string, Context> mContexts;
    };

}

#endif // COMPONENTS_LUA_I18N_H
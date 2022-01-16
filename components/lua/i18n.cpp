#include "i18n.hpp"

#include <components/debug/debuglog.hpp>

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::I18nManager::Context> : std::false_type {};
}

namespace LuaUtil
{

    void I18nManager::init()
    {
        mPreferredLanguages.push_back("en");
        sol::usertype<Context> ctx = mLua->sol().new_usertype<Context>("I18nContext");
        ctx[sol::meta_function::call] = &Context::translate;
        try
        {
            mI18nLoader = mLua->loadInternalLib("i18n");
            sol::set_environment(mLua->newInternalLibEnvironment(), mI18nLoader);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "LuaUtil::I18nManager initialization failed: " << e.what();
        }
    }

    void I18nManager::setPreferredLanguages(const std::vector<std::string>& langs)
    {
        {
            Log msg(Debug::Info);
            msg << "I18n preferred languages:";
            for (const std::string& l : langs)
                msg << " " << l;
        }
        mPreferredLanguages = langs;
        for (auto& [_, context] : mContexts)
            context.updateLang(this);
    }

    void I18nManager::Context::readLangData(I18nManager* manager, const std::string& lang)
    {
        std::string path = "i18n/";
        path.append(mName);
        path.append("/");
        path.append(lang);
        path.append(".lua");
        if (!manager->mVFS->exists(path))
            return;
        try
        {
            sol::protected_function dataFn = manager->mLua->loadFromVFS(path);
            sol::environment emptyEnv(manager->mLua->sol(), sol::create);
            sol::set_environment(emptyEnv, dataFn);
            sol::table data = manager->mLua->newTable();
            data[lang] = call(dataFn);
            call(mI18n["load"], data);
            mLoadedLangs[lang] = true;
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Can not load " << path << ": " << e.what();
        }
    }

    sol::object I18nManager::Context::translate(std::string_view key, const sol::object& data)
    {
        sol::object res = call(mI18n["translate"], key, data);
        if (res != sol::nil)
            return res;

        // If not found in a language file - register the key itself as a message.
        std::string composedKey = call(mI18n["getLocale"]).get<std::string>();
        composedKey.push_back('.');
        composedKey.append(key);
        call(mI18n["set"], composedKey, key);
        return call(mI18n["translate"], key, data);
    }

    void I18nManager::Context::updateLang(I18nManager* manager)
    {
        for (const std::string& lang : manager->mPreferredLanguages)
        {
            if (mLoadedLangs[lang] == sol::nil)
                readLangData(manager, lang);
            if (mLoadedLangs[lang] != sol::nil)
            {
                Log(Debug::Verbose) << "Language file \"i18n/" << mName << "/" << lang << ".lua\" is enabled";
                call(mI18n["setLocale"], lang);
                return;
            }
        }
        Log(Debug::Warning) << "No language files for the preferred languages found in \"i18n/" << mName << "\"";
    }

    sol::object I18nManager::getContext(const std::string& contextName)
    {
        if (mI18nLoader == sol::nil)
            throw std::runtime_error("LuaUtil::I18nManager is not initialized");
        auto it = mContexts.find(contextName);
        if (it != mContexts.end())
            return sol::make_object(mLua->sol(), it->second);
        Context ctx{contextName, mLua->newTable(), call(mI18nLoader, "i18n.init")};
        ctx.updateLang(this);
        mContexts.emplace(contextName, ctx);
        return sol::make_object(mLua->sol(), ctx);
    }

}

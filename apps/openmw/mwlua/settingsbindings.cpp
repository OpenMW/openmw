#include "luabindings.hpp"

#include <components/settings/settings.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/store.hpp"

namespace MWLua
{

    static sol::table initSettingsPackage(const Context& context, bool /*global*/, bool player)
    {
        LuaUtil::LuaState* lua = context.mLua;
        sol::table config(lua->sol(), sol::create);

        // Access to settings.cfg. Temporary, will be removed at some point.
        auto checkRead = [player](std::string_view category)
        {
            if ((category == "Camera" || category == "GUI" || category == "Hud" ||
                 category == "Windows" || category == "Input") && !player)
                throw std::runtime_error("This setting is only available in player scripts");
        };
        config["_getBoolFromSettingsCfg"] = [=](const std::string& category, const std::string& setting)
        {
            checkRead(category);
            return Settings::Manager::getBool(setting, category);
        };
        config["_getIntFromSettingsCfg"] = [=](const std::string& category, const std::string& setting)
        {
            checkRead(category);
            return Settings::Manager::getInt(setting, category);
        };
        config["_getFloatFromSettingsCfg"] = [=](const std::string& category, const std::string& setting)
        {
            checkRead(category);
            return Settings::Manager::getFloat(setting, category);
        };
        config["_getStringFromSettingsCfg"] = [=](const std::string& category, const std::string& setting)
        {
            checkRead(category);
            return Settings::Manager::getString(setting, category);
        };
        config["_getVector2FromSettingsCfg"] = [=](const std::string& category, const std::string& setting)
        {
            checkRead(category);
            return Settings::Manager::getVector2(setting, category);
        };
        config["_getVector3FromSettingsCfg"] = [=](const std::string& category, const std::string& setting)
        {
            checkRead(category);
            return Settings::Manager::getVector3(setting, category);
        };

        const MWWorld::Store<ESM::GameSetting>* gmst = &MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        config["getGMST"] = [lua, gmst](const std::string setting) -> sol::object
        {
            const ESM::Variant& value = gmst->find(setting)->mValue;
            if (value.getType() == ESM::VT_String)
                return sol::make_object<std::string>(lua->sol(), value.getString());
            else if (value.getType() == ESM::VT_Int)
                return sol::make_object<int>(lua->sol(), value.getInteger());
            else
                return sol::make_object<float>(lua->sol(), value.getFloat());
        };
        return LuaUtil::makeReadOnly(config);
    }

    sol::table initGlobalSettingsPackage(const Context& context) { return initSettingsPackage(context, true, false); }
    sol::table initLocalSettingsPackage(const Context& context) { return initSettingsPackage(context, false, false); }
    sol::table initPlayerSettingsPackage(const Context& context) { return initSettingsPackage(context, false, true); }

}

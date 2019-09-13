#include "gamesetting.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadgmst.hpp>

namespace MWLua
{
    void bindTES3GameSetting()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::GameSetting>();
        usertypeDefinition.set("new", sol::no_constructor);

        usertypeDefinition.set("id", &ESM::GameSetting::mId);
        usertypeDefinition.set("type", sol::readonly_property(
            [](ESM::GameSetting& self)
            {
                return self.mId[0];
            }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::GameSetting& self) -> sol::object
            {
                auto& luaManager = LuaManager::getInstance();
                auto stateHandle = luaManager.getThreadSafeStateHandle();
                sol::state& state = stateHandle.state;

                switch (self.mValue.getType())
                {
                    case ESM::VT_Int: return sol::make_object(state, self.mValue.getInteger());
                    case ESM::VT_Float: return sol::make_object(state, self.mValue.getFloat());
                    case ESM::VT_String: return sol::make_object(state, self.mValue.getString());
                    default: return sol::nil;
                }
            },
            [](ESM::GameSetting& self, sol::object value)
            {
                if (self.mValue.getType() == ESM::VT_String && value.is<std::string>())
                {
                    self.mValue.setString(value.as<std::string>());
                }
                else if (value.is<float>())
                {
                    if (self.mValue.getType() == ESM::VT_Int)
                    {
                        self.mValue.setInteger(value.as<float>());
                    }
                    else if (self.mValue.getType() == ESM::VT_Float)
                    {
                        self.mValue.setFloat(value.as<float>());
                    }
                }
            }
        ));

        // Allow object to be serialized to json.
        usertypeDefinition.set("__tojson", [](ESM::GameSetting& self, sol::table state)
        {
            std::ostringstream ss;
            ss << "\"tes3gameSetting:" << self.mId << "\"";
            return ss.str();
        });

        // Finish up our usertype.
        state.set_usertype("tes3gameSetting", usertypeDefinition);
    }
}

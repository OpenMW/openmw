#include "l10n.hpp"

#include <components/debug/debuglog.hpp>
#include <components/l10n/manager.hpp>
#include <components/lua/luastate.hpp>

namespace
{
    struct L10nContext
    {
        std::shared_ptr<const l10n::MessageBundles> mData;
    };

    void getICUArgs(std::string_view messageId, const sol::table& table, std::vector<icu::UnicodeString>& argNames,
        std::vector<icu::Formattable>& args)
    {
        for (auto& [key, value] : table)
        {
            // Argument values
            if (value.is<std::string>())
                args.push_back(icu::Formattable(LuaUtil::cast<std::string>(value).c_str()));
            // Note: While we pass all numbers as doubles, they still seem to be handled appropriately.
            // Numbers can be forced to be integers using the argType number and argStyle integer
            //     E.g. {var, number, integer}
            else if (value.is<double>())
                args.push_back(icu::Formattable(LuaUtil::cast<double>(value)));
            else
            {
                Log(Debug::Error) << "Unrecognized argument type for key \"" << LuaUtil::cast<std::string>(key)
                                  << "\" when formatting message \"" << messageId << "\"";
            }

            // Argument names
            const auto str = LuaUtil::cast<std::string>(key);
            argNames.push_back(
                icu::UnicodeString::fromUTF8(icu::StringPiece(str.data(), static_cast<int32_t>(str.size()))));
        }
    }
}

namespace sol
{
    template <>
    struct is_automagical<L10nContext> : std::false_type
    {
    };
}

namespace LuaUtil
{
    sol::function initL10nLoader(lua_State* L, l10n::Manager* manager)
    {
        sol::state_view lua(L);
        sol::usertype<L10nContext> ctxDef = lua.new_usertype<L10nContext>("L10nContext");
        ctxDef[sol::meta_function::call]
            = [](const L10nContext& ctx, std::string_view key, sol::optional<sol::table> args) {
                  std::vector<icu::Formattable> argValues;
                  std::vector<icu::UnicodeString> argNames;
                  if (args)
                      getICUArgs(key, *args, argNames, argValues);
                  return ctx.mData->formatMessage(key, argNames, argValues);
              };

        return sol::make_object(
            lua, [manager](const std::string& contextName, sol::optional<std::string> fallbackLocale) {
                if (fallbackLocale)
                    return L10nContext{ manager->getContext(contextName, *fallbackLocale) };
                else
                    return L10nContext{ manager->getContext(contextName) };
            });
    }
}

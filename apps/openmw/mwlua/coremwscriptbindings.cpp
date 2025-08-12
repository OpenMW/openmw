#include "coremwscriptbindings.hpp"

#include <components/esm3/loadscpt.hpp>

#include "../mwworld/esmstore.hpp"

#include "context.hpp"
#include "recordstore.hpp"

namespace MWLua
{
    sol::table initCoreMwScriptBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        auto recordBindingsClass = lua.new_usertype<ESM::Script>("ESM3_Script");
        recordBindingsClass[sol::meta_function::to_string]
            = [](const ESM::Script& rec) { return "ESM3_Script[" + rec.mId.toDebugString() + "]"; };
        recordBindingsClass["id"]
            = sol::readonly_property([](const ESM::Script& rec) { return rec.mId.serializeText(); });
        recordBindingsClass["text"]
            = sol::readonly_property([](const ESM::Script& rec) -> std::string_view { return rec.mScriptText; });

        addRecordFunctionBinding<ESM::Script>(api, context);

        return LuaUtil::makeReadOnly(api);
    }
}

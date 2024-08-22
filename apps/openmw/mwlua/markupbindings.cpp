#include "markupbindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/lua/yamlloader.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

#include "../mwbase/environment.hpp"

#include "context.hpp"

namespace MWLua
{
    sol::table initMarkupPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        api["loadYaml"] = [lua, vfs](std::string_view fileName) {
            Files::IStreamPtr file = vfs->get(VFS::Path::Normalized(fileName));
            return LuaUtil::loadYaml(*file, lua);
        };
        api["decodeYaml"]
            = [lua](std::string_view inputData) { return LuaUtil::loadYaml(std::string(inputData), lua); };

        return LuaUtil::makeReadOnly(api);
    }
}

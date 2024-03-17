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
        sol::table api(context.mLua->sol(), sol::create);

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        api["loadYaml"] = [lua = context.mLua, vfs](std::string_view fileName) {
            Files::IStreamPtr file = vfs->get(VFS::Path::Normalized(fileName));
            return LuaUtil::loadYaml(*file, lua->sol());
        };
        api["decodeYaml"] = [lua = context.mLua](std::string_view inputData) {
            return LuaUtil::loadYaml(std::string(inputData), lua->sol());
        };

        return LuaUtil::makeReadOnly(api);
    }
}

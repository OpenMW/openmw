#ifndef COMPONENTS_LUA_OMWSCRIPTSPARSER_H
#define COMPONENTS_LUA_OMWSCRIPTSPARSER_H

#include <components/vfs/manager.hpp>

namespace LuaUtil
{

    // Parses list of `*.omwscripts` files.
    std::vector<std::string> parseOMWScriptsFiles(const VFS::Manager* vfs, const std::vector<std::string>& scriptLists);

}

#endif // COMPONENTS_LUA_OMWSCRIPTSPARSER_H

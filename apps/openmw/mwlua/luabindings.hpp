#ifndef MWLUA_LUABINDINGS_H
#define MWLUA_LUABINDINGS_H

#include <components/lua/luastate.hpp>
#include <components/lua/serialization.hpp>
#include <components/lua/scriptscontainer.hpp>
#include <components/lua/storage.hpp>

#include "context.hpp"
#include "eventqueue.hpp"
#include "object.hpp"
#include "worldview.hpp"

namespace MWWorld
{
    class CellStore;
}

namespace MWLua
{

    sol::table initCorePackage(const Context&);
    sol::table initWorldPackage(const Context&);
    sol::table initPostprocessingPackage(const Context&);

    sol::table initGlobalStoragePackage(const Context&, LuaUtil::LuaStorage* globalStorage);
    sol::table initLocalStoragePackage(const Context&, LuaUtil::LuaStorage* globalStorage);
    sol::table initPlayerStoragePackage(const Context&, LuaUtil::LuaStorage* globalStorage, LuaUtil::LuaStorage* playerStorage);

    // Implemented in nearbybindings.cpp
    sol::table initNearbyPackage(const Context&);

    // Implemented in objectbindings.cpp
    void initObjectBindingsForLocalScripts(const Context&);
    void initObjectBindingsForGlobalScripts(const Context&);

    // Implemented in cellbindings.cpp
    void initCellBindingsForLocalScripts(const Context&);
    void initCellBindingsForGlobalScripts(const Context&);

    // Implemented in camerabindings.cpp
    sol::table initCameraPackage(const Context&);

    // Implemented in uibindings.cpp
    sol::table initUserInterfacePackage(const Context&);

    // Implemented in inputbindings.cpp
    sol::table initInputPackage(const Context&);

    // openmw.self package is implemented in localscripts.cpp
}

#endif // MWLUA_LUABINDINGS_H

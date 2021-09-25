#ifndef MWLUA_LUABINDINGS_H
#define MWLUA_LUABINDINGS_H

#include <components/lua/luastate.hpp>
#include <components/lua/serialization.hpp>
#include <components/lua/scriptscontainer.hpp>

#include "context.hpp"
#include "eventqueue.hpp"
#include "object.hpp"
#include "query.hpp"
#include "worldview.hpp"

namespace MWWorld
{
    class CellStore;
}

namespace MWLua
{

    sol::table initCorePackage(const Context&);
    sol::table initWorldPackage(const Context&);
    sol::table initQueryPackage(const Context&);

    sol::table initFieldGroup(const Context&, const QueryFieldGroup&);

    // Implemented in nearbybindings.cpp
    sol::table initNearbyPackage(const Context&);

    // Implemented in objectbindings.cpp
    void initObjectBindingsForLocalScripts(const Context&);
    void initObjectBindingsForGlobalScripts(const Context&);

    // Implemented in cellbindings.cpp
    struct LCell  // for local scripts
    {
        MWWorld::CellStore* mStore;
    };
    struct GCell  // for global scripts
    {
        MWWorld::CellStore* mStore;
    };
    void initCellBindingsForLocalScripts(const Context&);
    void initCellBindingsForGlobalScripts(const Context&);

    // Implemented in asyncbindings.cpp
    struct AsyncPackageId
    {
        LuaUtil::ScriptsContainer* mContainer;
        int mScriptId;
        sol::table mHiddenData;
    };
    sol::function getAsyncPackageInitializer(const Context&);

    // Implemented in camerabindings.cpp
    sol::table initCameraPackage(const Context&);

    // Implemented in uibindings.cpp
    sol::table initUserInterfacePackage(const Context&);

    // Implemented in inputbindings.cpp
    sol::table initInputPackage(const Context&);

    // Implemented in settingsbindings.cpp
    sol::table initGlobalSettingsPackage(const Context&);
    sol::table initLocalSettingsPackage(const Context&);
    sol::table initPlayerSettingsPackage(const Context&);

    // openmw.self package is implemented in localscripts.cpp
}

#endif // MWLUA_LUABINDINGS_H

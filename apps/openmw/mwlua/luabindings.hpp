#ifndef MWLUA_LUABINDINGS_H
#define MWLUA_LUABINDINGS_H

#include <components/lua/luastate.hpp>
#include <components/lua/serialization.hpp>
#include <components/lua/scriptscontainer.hpp>

#include "eventqueue.hpp"
#include "object.hpp"
#include "worldview.hpp"

namespace MWLua
{
    class LuaManager;

    struct Context
    {
        LuaManager* mLuaManager;
        LuaUtil::LuaState* mLua;
        LuaUtil::UserdataSerializer* mSerializer;
        WorldView* mWorldView;
        LocalEventQueue* mLocalEventQueue;
        GlobalEventQueue* mGlobalEventQueue;
    };

    sol::table initCorePackage(const Context&);
    sol::table initWorldPackage(const Context&);
    sol::table initNearbyPackage(const Context&);

    // Implemented in objectbindings.cpp
    void initObjectBindingsForLocalScripts(const Context&);
    void initObjectBindingsForGlobalScripts(const Context&);

    // Implemented in asyncbindings.cpp
    struct AsyncPackageId
    {
        // TODO: add ObjectId mLocalObject;
        LuaUtil::ScriptsContainer* mContainer;
        std::string mScript;
    };
    sol::function getAsyncPackageInitializer(const Context&);

    // Implemented in camerabindings.cpp
    sol::table initCameraPackage(const Context&);

    // Implemented in uibindings.cpp
    sol::table initUserInterfacePackage(const Context&);

    // openmw.self package is implemented in localscripts.cpp
}

#endif // MWLUA_LUABINDINGS_H

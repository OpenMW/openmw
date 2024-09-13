#include "luabindings.hpp"

#include <components/lua/asyncpackage.hpp>
#include <components/lua/utilpackage.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/datetimemanager.hpp"

#include "animationbindings.hpp"
#include "camerabindings.hpp"
#include "cellbindings.hpp"
#include "corebindings.hpp"
#include "debugbindings.hpp"
#include "inputbindings.hpp"
#include "localscripts.hpp"
#include "markupbindings.hpp"
#include "menuscripts.hpp"
#include "nearbybindings.hpp"
#include "objectbindings.hpp"
#include "postprocessingbindings.hpp"
#include "soundbindings.hpp"
#include "types/types.hpp"
#include "uibindings.hpp"
#include "vfsbindings.hpp"
#include "worldbindings.hpp"

namespace MWLua
{
    std::map<std::string, sol::object> initCommonPackages(const Context& context)
    {
        sol::state_view lua = context.mLua->unsafeState();
        MWWorld::DateTimeManager* tm = MWBase::Environment::get().getWorld()->getTimeManager();
        return {
            { "openmw.animation", initAnimationPackage(context) },
            { "openmw.async",
                LuaUtil::getAsyncPackageInitializer(
                    lua, [tm] { return tm->getSimulationTime(); }, [tm] { return tm->getGameTime(); }) },
            { "openmw.markup", initMarkupPackage(context) },
            { "openmw.util", LuaUtil::initUtilPackage(lua) },
            { "openmw.vfs", initVFSPackage(context) },
        };
    }

    std::map<std::string, sol::object> initGlobalPackages(const Context& context)
    {
        initObjectBindingsForGlobalScripts(context);
        initCellBindingsForGlobalScripts(context);
        return {
            { "openmw.core", initCorePackage(context) },
            { "openmw.types", initTypesPackage(context) },
            { "openmw.world", initWorldPackage(context) },
        };
    }

    std::map<std::string, sol::object> initLocalPackages(const Context& context)
    {
        initObjectBindingsForLocalScripts(context);
        initCellBindingsForLocalScripts(context);
        LocalScripts::initializeSelfPackage(context);
        return {
            { "openmw.core", initCorePackage(context) },
            { "openmw.types", initTypesPackage(context) },
            { "openmw.nearby", initNearbyPackage(context) },
        };
    }

    std::map<std::string, sol::object> initPlayerPackages(const Context& context)
    {
        return {
            { "openmw.ambient", initAmbientPackage(context) },
            { "openmw.camera", initCameraPackage(context.sol()) },
            { "openmw.debug", initDebugPackage(context) },
            { "openmw.input", initInputPackage(context) },
            { "openmw.postprocessing", initPostprocessingPackage(context) },
            { "openmw.ui", initUserInterfacePackage(context) },
        };
    }

    std::map<std::string, sol::object> initMenuPackages(const Context& context)
    {
        return {
            { "openmw.core", initCorePackage(context) },
            { "openmw.ambient", initAmbientPackage(context) },
            { "openmw.ui", initUserInterfacePackage(context) },
            { "openmw.menu", initMenuPackage(context) },
            { "openmw.input", initInputPackage(context) },
        };
    }
}

#include "navigator.hpp"
#include "navigatorimpl.hpp"
#include "navigatorstub.hpp"
#include "recastglobalallocator.hpp"

namespace DetourNavigator
{
    std::unique_ptr<Navigator> makeNavigator(const Settings& settings, const std::string& userDataPath)
    {
        DetourNavigator::RecastGlobalAllocator::init();

        std::unique_ptr<NavMeshDb> db;
        if (settings.mEnableNavMeshDiskCache)
            db = std::make_unique<NavMeshDb>(userDataPath + "/navmesh.db");

        return std::make_unique<NavigatorImpl>(settings, std::move(db));
    }

    std::unique_ptr<Navigator> makeNavigatorStub()
    {
        return std::make_unique<NavigatorStub>();
    }
}

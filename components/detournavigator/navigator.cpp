#include "navigator.hpp"
#include "navigatorimpl.hpp"
#include "navigatorstub.hpp"
#include "recastglobalallocator.hpp"

#include <components/debug/debuglog.hpp>

namespace DetourNavigator
{
    std::unique_ptr<Navigator> makeNavigator(const Settings& settings, const std::filesystem::path& userDataPath)
    {
        DetourNavigator::RecastGlobalAllocator::init();

        std::unique_ptr<NavMeshDb> db;
        if (settings.mEnableNavMeshDiskCache)
        {
            try
            {
                db = std::make_unique<NavMeshDb>((userDataPath / "navmesh.db").string(), settings.mMaxDbFileSize); //TODO(Project579): This will probably break in windows with unicode paths
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << e.what() << ", navigation mesh disk cache will be disabled";
            }
        }

        return std::make_unique<NavigatorImpl>(settings, std::move(db));
    }

    std::unique_ptr<Navigator> makeNavigatorStub()
    {
        return std::make_unique<NavigatorStub>();
    }
}

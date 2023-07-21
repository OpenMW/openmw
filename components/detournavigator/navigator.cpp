#include "navigator.hpp"
#include "navigatorimpl.hpp"
#include "navigatorstub.hpp"
#include "recastglobalallocator.hpp"

#include <components/debug/debuglog.hpp>
#include <components/files/conversion.hpp>

namespace DetourNavigator
{
    std::unique_ptr<Navigator> makeNavigator(const Settings& settings, const std::filesystem::path& userDataPath)
    {
        DetourNavigator::RecastGlobalAllocator::init();

        std::unique_ptr<NavMeshDb> db;
        if (settings.mEnableNavMeshDiskCache)
        {
            const std::string path = Files::pathToUnicodeString(userDataPath / "navmesh.db");
            Log(Debug::Info) << "Using " << path << " to store navigation mesh cache";
            try
            {
                db = std::make_unique<NavMeshDb>(path, settings.mMaxDbFileSize);
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

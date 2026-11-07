#include "navigator.hpp"
#include "navigatorimpl.hpp"
#include "navigatorstub.hpp"
#include "navmeshdb.hpp"
#include "recastglobalallocator.hpp"

#include <components/debug/debuglog.hpp>
#include <components/files/conversion.hpp>

#include <string>
#include <string_view>

namespace DetourNavigator
{
    namespace
    {
        void logNavMeshDbError(const Sqlite3::Error& error, std::string_view path)
        {
            Log(Debug::Error) << error.what() << ", navigation mesh disk cache will be disabled";
            if (const std::string advice = getNavMeshDbErrorAdvice(error, path); !advice.empty())
                Log(Debug::Error) << advice;
        }
    }

    std::unique_ptr<Navigator> makeNavigator(const Settings& settings, const std::filesystem::path& userDataPath)
    {
        DetourNavigator::RecastGlobalAllocator::init();

        std::unique_ptr<NavMeshDb> db;
        std::string path;
        if (settings.mEnableNavMeshDiskCache)
        {
            path = Files::pathToUnicodeString(userDataPath / "navmesh.db");
            Log(Debug::Info) << "Using " << path << " to store navigation mesh cache";
            try
            {
                db = std::make_unique<NavMeshDb>(path, settings.mMaxDbFileSize);
            }
            catch (const Sqlite3::Error& e)
            {
                logNavMeshDbError(e, path);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << e.what() << ", navigation mesh disk cache will be disabled";
            }
        }

        if (db != nullptr)
        {
            try
            {
                return std::make_unique<NavigatorImpl>(settings, std::move(db));
            }
            catch (const Sqlite3::Error& e)
            {
                logNavMeshDbError(e, path);
            }
        }

        return std::make_unique<NavigatorImpl>(settings, nullptr);
    }

    std::unique_ptr<Navigator> makeNavigatorStub()
    {
        return std::make_unique<NavigatorStub>();
    }
}

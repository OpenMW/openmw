#ifndef OPENMW_COMPONENTS_RESOURCE_CACHESATS
#define OPENMW_COMPONENTS_RESOURCE_CACHESATS

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace osg
{
    class Stats;
}

namespace Resource
{
    struct CacheStats
    {
        std::size_t mSize = 0;
        std::size_t mGet = 0;
        std::size_t mHit = 0;
        std::size_t mExpired = 0;
    };

    void addCacheStatsAttibutes(std::string_view prefix, std::vector<std::string>& out);

    void reportStats(std::string_view prefix, unsigned frameNumber, const CacheStats& src, osg::Stats& dst);
}

#endif

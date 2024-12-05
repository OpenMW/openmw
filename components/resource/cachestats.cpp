#include "cachestats.hpp"

#include <osg/Stats>

namespace Resource
{
    namespace
    {
        std::string makeAttribute(std::string_view prefix, std::string_view suffix)
        {
            std::string result;
            result.reserve(prefix.size() + 1 + suffix.size());
            result += prefix;
            result += ' ';
            result += suffix;
            return result;
        }
    }

    void addCacheStatsAttibutes(std::string_view prefix, std::vector<std::string>& out)
    {
        constexpr std::string_view suffixes[] = {
            "Count",
            "Get",
            "Hit",
            "Expired",
        };

        for (std::string_view suffix : suffixes)
            out.push_back(makeAttribute(prefix, suffix));
    }

    void reportStats(std::string_view prefix, unsigned frameNumber, const CacheStats& src, osg::Stats& dst)
    {
        dst.setAttribute(frameNumber, makeAttribute(prefix, "Count"), static_cast<double>(src.mSize));
        dst.setAttribute(frameNumber, makeAttribute(prefix, "Get"), static_cast<double>(src.mGet));
        dst.setAttribute(frameNumber, makeAttribute(prefix, "Hit"), static_cast<double>(src.mHit));
        dst.setAttribute(frameNumber, makeAttribute(prefix, "Expired"), static_cast<double>(src.mExpired));
    }
}

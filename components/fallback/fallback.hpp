#ifndef OPENMW_COMPONENTS_FALLBACK_H
#define OPENMW_COMPONENTS_FALLBACK_H

#include <map>
#include <string>

#include <osg/Vec4f>

namespace Fallback
{
    /// @brief contains settings imported from the Morrowind INI file.
    class Map
    {
            std::map<std::string,std::string> mFallbackMap;
        public:
            Map(const std::map<std::string,std::string>& fallback);
            Map() {}

            std::string getFallbackString(const std::string& fall) const;
            float getFallbackFloat(const std::string& fall) const;
            int getFallbackInt(const std::string& fall) const;
            bool getFallbackBool(const std::string& fall) const;
            osg::Vec4f getFallbackColour(const std::string& fall) const;
    };
}
#endif

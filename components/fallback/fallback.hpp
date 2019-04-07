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
            static std::map<std::string,std::string> mFallbackMap;
        public:
            static void init(const std::map<std::string,std::string>& fallback);

            static std::string getString(const std::string& fall);
            static float getFloat(const std::string& fall);
            static int getInt(const std::string& fall);
            static bool getBool(const std::string& fall);
            static osg::Vec4f getColour(const std::string& fall);
    };
}
#endif

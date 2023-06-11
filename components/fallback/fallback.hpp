#ifndef OPENMW_COMPONENTS_FALLBACK_H
#define OPENMW_COMPONENTS_FALLBACK_H

#include <map>
#include <string>
#include <string_view>

#include <osg/Vec4f>

namespace Fallback
{
    /// @brief contains settings imported from the Morrowind INI file.
    class Map
    {
        static std::map<std::string, int, std::less<>> mIntFallbackMap;
        static std::map<std::string, float, std::less<>> mFloatFallbackMap;
        static std::map<std::string, std::string, std::less<>> mNonNumericFallbackMap;

    public:
        static void init(const std::map<std::string, std::string>& fallback);

        static const std::map<std::string, int, std::less<>>& getIntFallbackMap() { return mIntFallbackMap; }
        static const std::map<std::string, float, std::less<>>& getFloatFallbackMap() { return mFloatFallbackMap; }
        static const std::map<std::string, std::string, std::less<>>& getNonNumericFallbackMap()
        {
            return mNonNumericFallbackMap;
        }

        static std::string_view getString(std::string_view fall);
        static float getFloat(std::string_view fall);
        static int getInt(std::string_view fall);
        static bool getBool(std::string_view fall);
        static osg::Vec4f getColour(std::string_view fall);
    };
}
#endif

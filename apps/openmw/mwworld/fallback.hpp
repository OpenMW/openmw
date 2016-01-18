#ifndef GAME_MWWORLD_FALLBACK_H
#define GAME_MWWORLD_FALLBACK_H

#include <map>
#include <string>

#include <osg/Vec4f>

namespace MWWorld
{
    class Fallback
    {
            const std::map<std::string,std::string> mFallbackMap;
        public:
            Fallback(const std::map<std::string,std::string>& fallback);
            std::string getFallbackString(const std::string& fall) const;
            float getFallbackFloat(const std::string& fall) const;
            int getFallbackInt(const std::string& fall) const;
            bool getFallbackBool(const std::string& fall) const;
            osg::Vec4f getFallbackColour(const std::string& fall) const;
    };
}
#endif

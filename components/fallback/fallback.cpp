#include "fallback.hpp"

#include <components/debug/debuglog.hpp>

namespace Fallback
{
    Map::Map(const std::map<std::string,std::string>& fallback):mFallbackMap(fallback)
    {}

    std::string Map::getFallbackString(const std::string& fall) const
    {
        std::map<std::string,std::string>::const_iterator it;
        if ((it = mFallbackMap.find(fall)) == mFallbackMap.end())
        {
            return std::string();
        }
        return it->second;
    }

    float Map::getFallbackFloat(const std::string& fall) const
    {
        std::string fallback = getFallbackString(fall);
        if (!fallback.empty())
        {
            try
            {
                return std::stof(fallback);
            }
            catch (const std::invalid_argument&)
            {
                Log(Debug::Error) << "Error: '" << fall << "' setting value (" << fallback << ") is not a valid number, using 0 as a fallback";
            }
        }

        return 0;
    }

    int Map::getFallbackInt(const std::string& fall) const
    {
        std::string fallback = getFallbackString(fall);
        if (!fallback.empty())
        {
            try
            {
                return std::stoi(fallback);
            }
            catch (const std::invalid_argument&)
            {
                Log(Debug::Error) << "Error: '" << fall << "' setting value (" << fallback << ") is not a valid number, using 0 as a fallback";
            }
        }

        return 0;
    }

    bool Map::getFallbackBool(const std::string& fall) const
    {
        return getFallbackString(fall) != "0";
    }

    osg::Vec4f Map::getFallbackColour(const std::string& fall) const
    {
        std::string sum = getFallbackString(fall);
        if (!sum.empty())
        {
            try
            {
                std::string ret[3];
                unsigned int j = 0;
                for (unsigned int i = 0; i < sum.length(); ++i)
                {
                    if(sum[i]==',') j++;
                    else if (sum[i] != ' ') ret[j]+=sum[i];
                }
                return osg::Vec4f(std::stoi(ret[0])/255.f,std::stoi(ret[1])/255.f,std::stoi(ret[2])/255.f, 1.f);    
            }
            catch (const std::invalid_argument&)
            {
                Log(Debug::Error) << "Error: '" << fall << "' setting value (" << sum << ") is not a valid color, using middle gray as a fallback";
            }
        }

        return osg::Vec4f(0.5f,0.5f,0.5f,1.f);
    }

}

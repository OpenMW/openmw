#include "fallback.hpp"

#include <sstream>

#include <components/debug/debuglog.hpp>

namespace Fallback
{
    std::map<std::string,std::string> Map::mFallbackMap;

    void Map::init(const std::map<std::string,std::string>& fallback)
    {
        mFallbackMap = fallback;
    }

    std::string Map::getString(const std::string& fall)
    {
        std::map<std::string,std::string>::const_iterator it;
        if ((it = mFallbackMap.find(fall)) == mFallbackMap.end())
        {
            return std::string();
        }
        return it->second;
    }

    float Map::getFloat(const std::string& fall)
    {
        const std::string& fallback = getString(fall);
        if (!fallback.empty())
        {
            std::stringstream stream(fallback);
            float number = 0.f;
            stream >> number;
            return number;
        }

        return 0;
    }

    int Map::getInt(const std::string& fall)
    {
        const std::string& fallback = getString(fall);
        if (!fallback.empty())
        {
            std::stringstream stream(fallback);
            int number = 0;
            stream >> number;
            return number;
        }

        return 0;
    }

    bool Map::getBool(const std::string& fall)
    {
        const std::string& fallback = getString(fall);
        return !fallback.empty() && fallback != "0";
    }

    osg::Vec4f Map::getColour(const std::string& fall)
    {
        const std::string& sum = getString(fall);
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

#include "fallback.hpp"

#include <boost/lexical_cast.hpp>


namespace Fallback
{
    bool stob(std::string const& s) {
        return s != "0";
    }

    Map::Map(const std::map<std::string,std::string>& fallback):mFallbackMap(fallback)
    {}

    std::string Map::getFallbackString(const std::string& fall) const
    {
        std::map<std::string,std::string>::const_iterator it;
        if((it = mFallbackMap.find(fall)) == mFallbackMap.end())
        {
            return "";
        }
        return it->second;
    }

    float Map::getFallbackFloat(const std::string& fall) const
    {
        std::string fallback=getFallbackString(fall);
        if (fallback.empty())
            return 0;
        else
            return boost::lexical_cast<float>(fallback);
    }

    int Map::getFallbackInt(const std::string& fall) const
    {
        std::string fallback=getFallbackString(fall);
        if (fallback.empty())
            return 0;
        else
            return std::stoi(fallback);
    }

    bool Map::getFallbackBool(const std::string& fall) const
    {
        std::string fallback=getFallbackString(fall);
        if (fallback.empty())
            return false;
        else
            return stob(fallback);
    }

    osg::Vec4f Map::getFallbackColour(const std::string& fall) const
    {
        std::string sum=getFallbackString(fall);
        if (sum.empty())
            return osg::Vec4f(0.5f,0.5f,0.5f,1.f);
        else
        {
            std::string ret[3];
            unsigned int j=0;
            for(unsigned int i=0;i<sum.length();++i){
                if(sum[i]==',') j++;
                else if (sum[i] != ' ') ret[j]+=sum[i];
            }

            return osg::Vec4f(std::stoi(ret[0])/255.f,std::stoi(ret[1])/255.f,std::stoi(ret[2])/255.f, 1.f);
        }
    }

}

#include "fallback.hpp"

#include <boost/lexical_cast.hpp>

namespace MWWorld
{
    Fallback::Fallback(const std::map<std::string,std::string>& fallback):mFallbackMap(fallback)
    {}

    std::string Fallback::getFallbackString(const std::string& fall) const
    {
        std::map<std::string,std::string>::const_iterator it;
        if((it = mFallbackMap.find(fall)) == mFallbackMap.end())
        {
            return "";
        }
        return it->second;
    }
    float Fallback::getFallbackFloat(const std::string& fall) const
    {
        std::string fallback=getFallbackString(fall);
        if(fallback.empty())
            return 0;
        else
            return boost::lexical_cast<float>(fallback);
    }
    int Fallback::getFallbackInt(const std::string& fall) const
    {
        std::string fallback=getFallbackString(fall);
        if(fallback.empty())
            return 0;
        else
            return boost::lexical_cast<int>(fallback);
    }

    bool Fallback::getFallbackBool(const std::string& fall) const
    {
        std::string fallback=getFallbackString(fall);
        if(fallback.empty())
            return false;
        else
            return boost::lexical_cast<bool>(fallback);
    }
    osg::Vec4f Fallback::getFallbackColour(const std::string& fall) const
    {
        std::string sum=getFallbackString(fall);
        if(sum.empty())
            return osg::Vec4f(0.f,0.f,0.f,1.f);
        else
        {
            std::string ret[3];
            unsigned int j=0;
            for(unsigned int i=0;i<sum.length();++i){
                if(sum[i]==',') j++;
                else if (sum[i] != ' ') ret[j]+=sum[i];
            }

            return osg::Vec4f(boost::lexical_cast<int>(ret[0])/255.f,boost::lexical_cast<int>(ret[1])/255.f,boost::lexical_cast<int>(ret[2])/255.f, 1.f);
        }
    }

}

#ifndef OPENMW_COMPONENTS_FALLBACK_VALIDATE_H
#define OPENMW_COMPONENTS_FALLBACK_VALIDATE_H

#include <boost/program_options.hpp>

// Parses and validates a fallback map from boost program_options.
// Note: for boost to pick up the validate function, you need to pull in the namespace e.g.
// by using namespace Fallback;

namespace Fallback
{

    struct FallbackMap {
        std::map<std::string,std::string> mMap;
    };

    void validate(boost::any &v, std::vector<std::string> const &tokens, FallbackMap*, int)
    {
        if(v.empty())
        {
            v = boost::any(FallbackMap());
        }

        FallbackMap *map = boost::any_cast<FallbackMap>(&v);

        for(std::vector<std::string>::const_iterator it=tokens.begin(); it != tokens.end(); ++it)
        {
            int sep = it->find(",");
            if(sep < 1 || sep == (int)it->length()-1)
    #if (BOOST_VERSION < 104200)
                throw boost::program_options::validation_error("invalid value");
    #else
                throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
    #endif

            std::string key(it->substr(0,sep));
            std::string value(it->substr(sep+1));

            if(map->mMap.find(key) == map->mMap.end())
            {
                map->mMap.insert(std::make_pair (key,value));
            }
        }
    }

}

#endif

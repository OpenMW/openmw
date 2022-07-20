#ifndef OPENMW_COMPONENTS_FALLBACK_VALIDATE_H
#define OPENMW_COMPONENTS_FALLBACK_VALIDATE_H

#include <map>
#include <string>
#include <vector>

// Parses and validates a fallback map from boost program_options.
// Note: for boost to pick up the validate function, you need to pull in the namespace e.g.
// by using namespace Fallback;

namespace boost
{
    class any;
}

namespace Fallback
{

    struct FallbackMap {
        std::map<std::string, std::string> mMap;
    };

    void validate(boost::any &v, std::vector<std::string> const &tokens, FallbackMap*, int);
}

#endif

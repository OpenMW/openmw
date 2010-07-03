
#include "extensions.hpp"

namespace Compiler
{

    Extensions::Extensions() : mNextKeywordIndex (-1) {}

    int Extensions::searchKeyword (const std::string& keyword) const
    {
        std::map<std::string, int>::const_iterator iter = mKeywords.find (keyword);
        
        if (iter==mKeywords.end())
            return 0;
            
        return iter->second;
    }
}

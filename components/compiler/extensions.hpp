#ifndef COMPILER_EXTENSIONS_H_INCLUDED
#define COMPILER_EXTENSINOS_H_INCLUDED

#include <string>
#include <map>

namespace Compiler
{
    /// \brief Collection of compiler extensions
    
    class Extensions
    {
            int mNextKeywordIndex;
            std::map<std::string, int> mKeywords;
    
        public:
    
            Extensions();
    
            int searchKeyword (const std::string& keyword) const;
            ///< Return extension keyword code, that is assigned to the string \a keyword.
            /// - if no match is found 0 is returned.
            /// - keyword must be all lower case.
    };
}

#endif


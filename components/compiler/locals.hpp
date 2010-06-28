#ifndef COMPILER_LOCALS_H_INCLUDED
#define COMPILER_LOCALS_H_INCLUDED

#include <vector>
#include <string>

namespace Compiler
{
    /// \brief Local variable declarations
    
    class Locals
    {
            std::vector<std::string> mShorts;
            std::vector<std::string> mLongs;
            std::vector<std::string> mFloats;
    
            const std::vector<std::string>& get (char type) const;

            bool search (char type, const std::string& name) const;
            
            std::vector<std::string>& get (char type);   
    
        public:
    
            char getType (const std::string& name) const;
            ///< 's': short, 'l': long, 'f': float, ' ': does not exist.
            
            void declare (char type, const std::string& name);
            ///< declares a variable.
            
            void clear();
            ///< remove all declarations.
    };
}

#endif

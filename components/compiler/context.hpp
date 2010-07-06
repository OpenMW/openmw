#ifndef COMPILER_CONTEXT_H_INCLUDED
#define COMPILER_CONTEXT_H_INCLUDED

#include <string>

namespace Compiler
{
    class Extensions;

    class Context
    {
            const Extensions *mExtensions;
            
        public:
        
            Context() : mExtensions (0) {}
        
            virtual ~Context() {}
            
            virtual bool canDeclareLocals() const = 0;
            ///< Is the compiler allowed to declare local variables?
            
            void setExtensions (const Extensions *extensions = 0)
            {
                mExtensions = extensions;
            }
            
            const Extensions *getExtensions() const
            {
                return mExtensions;
            }
            
            virtual char getGlobalType (const std::string& name) const = 0;
            ///< 'l: long, 's': short, 'f': float, ' ': does not exist.           
    };
}

#endif


#ifndef COMPILER_CONTEXT_H_INCLUDED
#define COMPILER_CONTEXT_H_INCLUDED

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
            
            void setExtensions (const Extensions *extensions = 0);
            ///< Set compiler extensions.
    };
}

#endif


#ifndef COMPILER_CONTEXT_H_INCLUDED
#define COMPILER_CONTEXT_H_INCLUDED

namespace Compiler
{
    class Context
    {
        public:
        
            virtual ~Context() {}
            
            virtual bool canDeclareLocals() const = 0;
            ///< Is the compiler allowed to declare local variables?
    };
}

#endif


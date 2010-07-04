#ifndef MWCOMPILER_CONTEXT_H_INCLUDED
#define MWCOMPILER_CONTEXT_H_INCLUDED

#include <components/compiler/context.hpp>

namespace SACompiler
{
    class Context : public Compiler::Context
    {
        public:
        
            virtual bool canDeclareLocals() const;
            ///< Is the compiler allowed to declare local variables?    
            
            virtual char getGlobalType (const std::string& name) const;
            ///< 'l: long, 's': short, 'f': float, ' ': does not exist.              
    };
}

#endif


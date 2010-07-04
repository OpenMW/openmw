#ifndef GAME_SCRIPT_COMPILERCONTEXT_H
#define GAME_SCRIPT_COMPILERCONTEXT_H

#include <components/compiler/context.hpp>

namespace MWScript
{
    class CompilerContext : public Compiler::Context
    {
        public:
        
            enum Type
            {
                Type_Full, // global, local, targetted
                Type_Dialgoue,
                Type_Console
            };
            
        private:
        
            Type mType;
    
        public:
        
            CompilerContext (Type type) : mType (type) {}
        
            // Is the compiler allowed to declare local variables?
            virtual bool canDeclareLocals() const
            {
                return mType==Type_Full;
            }    
    };
}

#endif



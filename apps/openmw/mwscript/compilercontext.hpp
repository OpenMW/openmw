#ifndef GAME_SCRIPT_COMPILERCONTEXT_H
#define GAME_SCRIPT_COMPILERCONTEXT_H

#include <components/compiler/context.hpp>

namespace MWWorld
{
    class Environment;
}

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
            const MWWorld::Environment& mEnvironment;
    
        public:
        
            CompilerContext (Type type, const MWWorld::Environment& environment);
        
            /// Is the compiler allowed to declare local variables?
            virtual bool canDeclareLocals() const;  
            
            /// 'l: long, 's': short, 'f': float, ' ': does not exist.              
            virtual char getGlobalType (const std::string& name) const;

            virtual bool isId (const std::string& name) const;
            ///< Does \a name match an ID, that can be referenced?            
    };
}

#endif



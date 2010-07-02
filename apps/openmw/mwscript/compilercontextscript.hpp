#ifndef GAME_SCRIPT_COMPILERCONTEXTSCRIPT_H
#define GAME_SCRIPT_COMPILERCONTEXTSCRIPT_H

#include "compilercontext.hpp"

namespace MWScript
{
    /// Context for local scripts, global scripts and targetted scripts

    class CompilerContextScript : public CompilerContext
    {
        public:
        
            // Is the compiler allowed to declare local variables?
            virtual bool canDeclareLocals() const
            {
                return true;
            }
    };
}

#endif

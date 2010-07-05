#ifndef INTERPRETER_SPATIALOPCODES_H_INCLUDED
#define INTERPRETER_SPATIALOPCODES_H_INCLUDED

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{    
    class OpGetDistance : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                std::string name = runtime.getStringLiteral (runtime[0]);            
                runtime[0] = runtime.getContext().getDistance (name);
            }            
    };
}

#endif


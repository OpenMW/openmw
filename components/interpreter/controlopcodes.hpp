#ifndef INTERPRETER_CONTROLOPCODES_H_INCLUDED
#define INTERPRETER_CONTROLOPCODES_H_INCLUDED

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{
    class OpReturn : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                runtime.setPC (-1);
            }           
    };
}

#endif

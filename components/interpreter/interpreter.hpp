#ifndef INTERPRETER_INTERPRETER_H_INCLUDED
#define INTERPRETER_INTERPRETER_H_INCLUDED

#include "runtime.hpp"
#include "types.hpp"

namespace Interpreter
{
    class Interpreter
    {
            Runtime mRuntime;
            
            // not implemented
            Interpreter (const Interpreter&);
            Interpreter& operator= (const Interpreter&);
            
        public:
        
            Interpreter (Context& context);
            
            ~Interpreter();
            
            void run (const Type_Code *code, int codeSize);
    };
}

#endif


#ifndef INTERPRETER_OPCODES_H_INCLUDED
#define INTERPRETER_OPCODES_H_INCLUDED

namespace Interpreter
{
    class Runtime;

    /// opcode for 0 arguments
    class Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime) = 0;
            
            virtual ~Opcode0() {}
    };

    /// opcode for 1 argument
    class Opcode1
    {
        public:
        
            virtual void execute (Runtime& runtime, unsigned int arg0) = 0;
            
            virtual ~Opcode1() {}
    };

    /// opcode for 2 arguments
    class Opcode2
    {
        public:
        
            virtual void execute (Runtime& runtime, unsigned int arg1, unsigned int arg2) = 0;
            
            virtual ~Opcode2() {}
    };

}

#endif

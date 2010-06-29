#ifndef INTERPRETER_GENERICOPCODES_H_INCLUDED
#define INTERPRETER_GENERICOPCODES_H_INCLUDED

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{
    class OpPushInt : public Opcode1
    {
        public:
        
            virtual void execute (Runtime& runtime, unsigned int arg0)
            {
                runtime.push (arg0);
            }        
    };
    
    class OpIntToFloat : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Integer data = *reinterpret_cast<Type_Integer *> (&runtime[0]);
                Type_Float floatValue = static_cast<Type_Float> (data);
                runtime[0] = *reinterpret_cast<Type_Data *> (&floatValue);
            }           
    };
    
    class OpFloatToInt : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Float data = *reinterpret_cast<Type_Float *> (&runtime[0]);
                Type_Integer integerValue = static_cast<Type_Integer> (data);
                runtime[0] = *reinterpret_cast<Type_Data *> (&integerValue);
            }           
    };    
}

#endif


#ifndef INTERPRETER_GENERICOPCODES_H_INCLUDED
#define INTERPRETER_GENERICOPCODES_H_INCLUDED

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{
    class OpPushInt : public Opcode1
    {
        public:
        
            void execute (Runtime& runtime, unsigned int arg0) override
            {
                runtime.push (static_cast<Type_Integer> (arg0));
            }        
    };
    
    class OpIntToFloat : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                Type_Float floatValue = static_cast<Type_Float> (data);
                runtime[0].mFloat = floatValue;
            }           
    };
    
    class OpFloatToInt : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                Type_Float data = runtime[0].mFloat;
                Type_Integer integerValue = static_cast<Type_Integer> (data);
                runtime[0].mInteger = integerValue;
            }           
    };    
    
    class OpNegateInt : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                data = -data;
                runtime[0].mInteger = data;
            }           
    };    
    
    class OpNegateFloat : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                Type_Float data = runtime[0].mFloat;
                data = -data;
                runtime[0].mFloat = data;
            }           
    };
    
    class OpIntToFloat1 : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[1].mInteger;
                Type_Float floatValue = static_cast<Type_Float> (data);
                runtime[1].mFloat = floatValue;
            }           
    };
    
    class OpFloatToInt1 : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                Type_Float data = runtime[1].mFloat;
                Type_Integer integerValue = static_cast<Type_Integer> (data);
                runtime[1].mInteger = integerValue;
            }           
    };            
}

#endif


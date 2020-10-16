#ifndef INTERPRETER_MATHOPCODES_H_INCLUDED
#define INTERPRETER_MATHOPCODES_H_INCLUDED

#include <stdexcept>
#include <cmath>

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{
    template<typename T>
    class OpAddInt : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                T result = getData<T> (runtime[1]) + getData<T> (runtime[0]);
                
                runtime.pop();
                
                getData<T> (runtime[0]) = result;
            }           
    };

    template<typename T>
    class OpSubInt : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                T result = getData<T> (runtime[1]) - getData<T> (runtime[0]);
                
                runtime.pop();

                getData<T> (runtime[0]) = result;
            }           
    };

    template<typename T>
    class OpMulInt : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                T result = getData<T> (runtime[1]) * getData<T> (runtime[0]);
                
                runtime.pop();

                getData<T> (runtime[0]) = result;
            }           
    };

    template<typename T>
    class OpDivInt : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                T left = getData<T> (runtime[0]);
            
                if (left==0)
                    throw std::runtime_error ("division by zero");
            
                T result = getData<T> (runtime[1]) / left;
                
                runtime.pop();

                getData<T> (runtime[0]) = result;
            }           
    };
    
    class OpSquareRoot : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                Type_Float value = runtime[0].mFloat;
                
                if (value<0)
                    throw std::runtime_error (
                        "square root of negative number (we aren't that imaginary)");
                
                value = std::sqrt (value);
                
                runtime[0].mFloat = value;
            }           
    };    
    
    template<typename T, typename C>
    class OpCompare : public Opcode0
    {
        public:
        
            void execute (Runtime& runtime) override
            {
                int result = C() (getData<T> (runtime[1]), getData<T> (runtime[0]));
                
                runtime.pop();
                
                runtime[0].mInteger = result;
            }           
    };    
}

#endif


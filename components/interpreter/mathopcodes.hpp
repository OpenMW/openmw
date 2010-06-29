#ifndef INTERPRETER_MATHOPCODES_H_INCLUDED
#define INTERPRETER_MATHOPCODES_H_INCLUDED

#include <stdexcept>

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{
    template<typename T>
    class OpAddInt : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                T result =
                    *reinterpret_cast<T *> (&runtime[1])
                    + 
                    *reinterpret_cast<T *> (&runtime[0]);
                
                runtime.pop();
                
                runtime[0] = *reinterpret_cast<Type_Data *> (&result);
            }           
    };

    template<typename T>
    class OpSubInt : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                T result =
                    *reinterpret_cast<T *> (&runtime[1])
                    - 
                    *reinterpret_cast<T *> (&runtime[0]);
                
                runtime.pop();
                
                runtime[0] = *reinterpret_cast<Type_Data *> (&result);
            }           
    };

    template<typename T>
    class OpMulInt : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                T result =
                    *reinterpret_cast<T *> (&runtime[1])
                    * 
                    *reinterpret_cast<T *> (&runtime[0]);
                
                runtime.pop();
                
                runtime[0] = *reinterpret_cast<Type_Data *> (&result);
            }           
    };

    template<typename T>
    class OpDivInt : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                T left = *reinterpret_cast<T *> (&runtime[0]);
            
                if (left==0)
                    throw std::runtime_error ("division by zero");
            
                T result =
                    *reinterpret_cast<T *> (&runtime[1])
                    / 
                    left;
                
                runtime.pop();
                
                runtime[0] = *reinterpret_cast<Type_Data *> (&result);
            }           
    };
}

#endif


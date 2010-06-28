#ifndef INTERPRETER_LOCALOPCODES_H_INCLUDED
#define INTERPRETER_LOCALOPCODES_H_INCLUDED

#include "opcodes.hpp"
#include "runtime.hpp"
#include "context.hpp"

namespace Interpreter
{   
    class OpStoreLocalShort : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Data data = runtime[0];
                int index = runtime[1];

                runtime.getContext().setLocalShort (index, *reinterpret_cast<int *> (&data));

                runtime.pop();
                runtime.pop();
            }           
    };
    
    class OpStoreLocalLong : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Data data = runtime[0];
                int index = runtime[1];

                runtime.getContext().setLocalLong (index, *reinterpret_cast<int *> (&data));

                runtime.pop();
                runtime.pop();
            }           
    };    
    
    class OpStoreLocalFloat : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Data data = runtime[0];
                int index = runtime[1];

                runtime.getContext().setLocalFloat (index, *reinterpret_cast<float *> (&data));

                runtime.pop();
                runtime.pop();
            }           
    };
    
    class OpFetchIntLiteral : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int intValue = runtime.getIntegerLiteral (runtime[0]);
                runtime[0] = intValue;
            }           
    };    
    
    class OpFetchFloatLiteral : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                float floatValue = runtime.getFloatLiteral (runtime[0]);
                runtime[0] = *reinterpret_cast<Type_Data *> (&floatValue);
            }           
    };     
}

#endif


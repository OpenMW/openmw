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
    
    class OpFetchLocalShort : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0];           
                int value = runtime.getContext().getLocalShort (index);
                runtime[0] = *reinterpret_cast<Type_Data *> (&value);
            }           
    };    

    class OpFetchLocalLong : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0];           
                int value = runtime.getContext().getLocalLong (index);
                runtime[0] = *reinterpret_cast<Type_Data *> (&value);
            }           
    };    

    class OpFetchLocalFloat : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0];           
                float value = runtime.getContext().getLocalFloat (index);
                runtime[0] = *reinterpret_cast<Type_Data *> (&value);
            }           
    };    
    
    class OpStoreGlobalShort : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Data data = runtime[0];
                int index = runtime[1];

                std::string name = runtime.getStringLiteral (index);

                runtime.getContext().setGlobalShort (name, *reinterpret_cast<int *> (&data));

                runtime.pop();
                runtime.pop();
            }           
    };
    
    class OpStoreGlobalLong : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Data data = runtime[0];
                int index = runtime[1];

                std::string name = runtime.getStringLiteral (index);

                runtime.getContext().setGlobalLong (name, *reinterpret_cast<int *> (&data));

                runtime.pop();
                runtime.pop();
            }           
    };    
    
    class OpStoreGlobalFloat : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                Type_Data data = runtime[0];
                int index = runtime[1];

                std::string name = runtime.getStringLiteral (index);

                runtime.getContext().setGlobalFloat (name, *reinterpret_cast<float *> (&data));

                runtime.pop();
                runtime.pop();
            }           
    };
    
    class OpFetchGlobalShort : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0]; 
                std::string name = runtime.getStringLiteral (index);
                int value = runtime.getContext().getGlobalShort (name);
                runtime[0] = *reinterpret_cast<Type_Data *> (&value);
            }           
    };    

    class OpFetchGlobalLong : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0];           
                std::string name = runtime.getStringLiteral (index);
                int value = runtime.getContext().getGlobalLong (name);
                runtime[0] = *reinterpret_cast<Type_Data *> (&value);
            }           
    };    

    class OpFetchGlobalFloat : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0];           
                std::string name = runtime.getStringLiteral (index);
                float value = runtime.getContext().getGlobalFloat (name);
                runtime[0] = *reinterpret_cast<Type_Data *> (&value);
            }           
    };        
}

#endif


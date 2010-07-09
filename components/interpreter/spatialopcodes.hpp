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
                
                float distance = runtime.getContext().getDistance (name);
                
                runtime[0] = *reinterpret_cast<Type_Data *> (&distance);
            }            
    };
    
    class OpGetDistanceExplicit : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0];
                runtime.pop();
                std::string id = runtime.getStringLiteral (index);
                            
                std::string name = runtime.getStringLiteral (runtime[0]);            
                
                float distance = runtime.getContext().getDistance (name, id);
                
                runtime[0] = *reinterpret_cast<Type_Data *> (&distance);
            }            
    };    
}

#endif


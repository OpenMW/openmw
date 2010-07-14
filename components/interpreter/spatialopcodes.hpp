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
                std::string name = runtime.getStringLiteral (runtime[0].mInteger);            
                
                Type_Float distance = runtime.getContext().getDistance (name);
                
                runtime[0].mFloat = distance;
            }            
    };
    
    class OpGetDistanceExplicit : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0].mInteger;
                runtime.pop();
                std::string id = runtime.getStringLiteral (index);
                            
                std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                
                Type_Float distance = runtime.getContext().getDistance (name, id);
                
                runtime[0].mFloat = distance;
            }            
    };    
}

#endif


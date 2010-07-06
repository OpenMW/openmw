
#include "miscextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Misc
    {
        class OpXBox : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (0);
                } 
        };
    
        class OpOnActivate : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (static_cast<InterpreterContext&> (
                        runtime.getContext()).hasBeenActivated());
                } 
        };
    
        const int opcodeXBox = 0x200000c;
        const int opcodeOnActivate = 0x200000d;       
    
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("xbox", 'l', "", opcodeXBox);
            extensions.registerFunction ("onactivate", 'l', "", opcodeOnActivate);
        }
        
        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeXBox, new OpXBox);
            interpreter.installSegment5 (opcodeOnActivate, new OpOnActivate);
        }    
    }        
}

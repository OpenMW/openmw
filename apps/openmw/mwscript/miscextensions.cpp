
#include "miscextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

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
    
        const int opcodeXBox = 0x200000c;
        

    
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("xbox", 'l', "", opcodeXBox);
        }
        
        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeXBox, new OpXBox);
        }    
    }        
}

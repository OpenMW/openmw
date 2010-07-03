
#include "cellextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../world.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Cell
    {
        class OpCellChanged : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());
                        
                    runtime.push (context.getWorld().hasCellChanged() ? 1 : 0);
                } 
        };
    
        const int opcodeCellChanged = 0x2000000;
    
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("cellchanged", 'l', "", opcodeCellChanged);
        }
        
        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeCellChanged, new OpCellChanged);
        }
    }
}


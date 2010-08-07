
#include "containerextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Container
    {
        class OpAddItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    MWWorld::ManualRef ref (context.getWorld().getStore(), item);

                    ref.getPtr().getRefData().setCount (count);

                    MWWorld::Class::get (ref.getPtr()).insertIntoContainer (ref.getPtr(),
                        MWWorld::Class::get (ptr).getContainerStore (ptr));
                }
        };

        class OpAddItemExplicit : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWWorld::ManualRef ref (context.getWorld().getStore(), item);

                    ref.getPtr().getRefData().setCount (count);

                    MWWorld::Class::get (ref.getPtr()).insertIntoContainer (ref.getPtr(),
                        MWWorld::Class::get (ptr).getContainerStore (ptr));
                }
        };

        const int opcodeAddItem = 0x2000076;
        const int opcodeAddItemExplicit = 0x2000077;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("additem", "cl", opcodeAddItem, opcodeAddItemExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
             interpreter.installSegment5 (opcodeAddItem, new OpAddItem);
             interpreter.installSegment5 (opcodeAddItemExplicit, new OpAddItemExplicit);
        }
    }
}

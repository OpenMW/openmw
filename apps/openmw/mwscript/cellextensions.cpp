
#include "cellextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/world.hpp"

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

        class OpCOC : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    std::string cell = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    ESM::Position pos;
                    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
                    pos.pos[2] = 0;

                    if (const ESM::Cell *exterior = context.getWorld().getExterior (cell))
                    {
                        context.getWorld().indexToPosition (exterior->data.gridX, exterior->data.gridY,
                            pos.pos[0], pos.pos[1], true);
                        context.getWorld().changeToExteriorCell (pos);
                    }
                    else
                    {
                        pos.pos[0] = pos.pos[1] = 0;
                        context.getWorld().changeCell (cell, pos);
                    }
                }
        };

        class OpCOE : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer x = runtime[0].mInteger;
                    runtime.pop();

                    Interpreter::Type_Integer y = runtime[0].mInteger;
                    runtime.pop();

                    ESM::Position pos;

                    context.getWorld().indexToPosition (x, y, pos.pos[0], pos.pos[1], true);
                    pos.pos[2] = 0;

                    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

                    context.getWorld().changeToExteriorCell (pos);
                }
        };

        const int opcodeCellChanged = 0x2000000;
        const int opcodeCOC = 0x2000026;
        const int opcodeCOE = 0x200008e;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("cellchanged", 'l', "", opcodeCellChanged);
            extensions.registerInstruction ("coc", "S", opcodeCOC);
            extensions.registerInstruction ("centeroncell", "S", opcodeCOC);
            extensions.registerInstruction ("coe", "ll", opcodeCOE);
            extensions.registerInstruction ("centeronexterior", "ll", opcodeCOE);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeCellChanged, new OpCellChanged);
            interpreter.installSegment5 (opcodeCOC, new OpCOC);
            interpreter.installSegment5 (opcodeCOE, new OpCOE);
        }
    }
}

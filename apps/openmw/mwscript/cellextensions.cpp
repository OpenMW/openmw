
#include "cellextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/world.hpp"
#include "../mwworld/player.hpp"

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
                        context.getWorld().changeToInteriorCell (cell, pos);
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

        class OpGetInterior : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    bool interior =
                        context.getWorld().getPlayer().getPlayer().getCell()->cell->data.flags &
                            ESM::Cell::Interior;

                    runtime.push (interior ? 1 : 0);
                }
        };

        class OpGetPCCell : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    const ESM::Cell *cell = context.getWorld().getPlayer().getPlayer().getCell()->cell;

                    std::string current = cell->name;

                    if (!(cell->data.flags & ESM::Cell::Interior) && current.empty())
                    {
                        const ESM::Region *region =
                            context.getWorld().getStore().regions.find (cell->region);

                        current = region->name;
                    }

                    bool match = current.length()>=name.length() &&
                        current.substr (0, name.length())==name;

                    runtime.push (match ? 1 : 0);
                }
        };

        class OpGetWaterLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    const ESM::Cell *cell = context.getWorld().getPlayer().getPlayer().getCell()->cell;
                    runtime.push (cell->water);
                }
        };

        class OpSetWaterLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Float level = runtime[0].mFloat;

                    MWWorld::Ptr::CellStore *cell = context.getWorld().getPlayer().getPlayer().getCell();
                    cell->mWaterLevel = level;
                    context.getEnvironment().mWorld->setWaterHeight(cell->mWaterLevel);
                }
        };

        class OpModWaterLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Float level = runtime[0].mFloat;

                    MWWorld::Ptr::CellStore *cell = context.getWorld().getPlayer().getPlayer().getCell();
                    cell->mWaterLevel +=level;
                    context.getEnvironment().mWorld->setWaterHeight(cell->mWaterLevel);
                }
        };

        const int opcodeCellChanged = 0x2000000;
        const int opcodeCOC = 0x2000026;
        const int opcodeCOE = 0x200008e;
        const int opcodeGetInterior = 0x2000131;
        const int opcodeGetPCCell = 0x2000136;
        const int opcodeGetWaterLevel = 0x2000141;
        const int opcodeSetWaterLevel = 0x2000142;
        const int opcodeModWaterLevel = 0x2000143;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("cellchanged", 'l', "", opcodeCellChanged);
            extensions.registerInstruction ("coc", "S", opcodeCOC);
            extensions.registerInstruction ("centeroncell", "S", opcodeCOC);
            extensions.registerInstruction ("coe", "ll", opcodeCOE);
            extensions.registerInstruction ("centeronexterior", "ll", opcodeCOE);
            extensions.registerInstruction ("setwaterlevel", "f", opcodeSetWaterLevel);
            extensions.registerInstruction ("modwaterlevel", "f", opcodeModWaterLevel);
            extensions.registerFunction ("getinterior", 'l', "", opcodeGetInterior);
            extensions.registerFunction ("getpccell", 'l', "c", opcodeGetPCCell);
            extensions.registerFunction ("getwaterlevel", 'f', "", opcodeGetWaterLevel);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeCellChanged, new OpCellChanged);
            interpreter.installSegment5 (opcodeCOC, new OpCOC);
            interpreter.installSegment5 (opcodeCOE, new OpCOE);
            interpreter.installSegment5 (opcodeGetInterior, new OpGetInterior);
            interpreter.installSegment5 (opcodeGetPCCell, new OpGetPCCell);
            interpreter.installSegment5 (opcodeGetWaterLevel, new OpGetWaterLevel);
            interpreter.installSegment5 (opcodeSetWaterLevel, new OpSetWaterLevel);
            interpreter.installSegment5 (opcodeModWaterLevel, new OpModWaterLevel);
        }
    }
}

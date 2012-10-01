
#include "cellextensions.hpp"

#include <components/esm_store/store.hpp>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

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
                    runtime.push (MWBase::Environment::get().getWorld()->hasCellChanged() ? 1 : 0);
                }
        };

        class OpCOC : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string cell = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    ESM::Position pos;
                    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
                    pos.pos[2] = 0;

                    if (const ESM::Cell *exterior = MWBase::Environment::get().getWorld()->getExterior (cell))
                    {
                        MWBase::Environment::get().getWorld()->indexToPosition (exterior->mData.mX, exterior->mData.mY,
                            pos.pos[0], pos.pos[1], true);
                        MWBase::Environment::get().getWorld()->changeToExteriorCell (pos);
                    }
                    else
                    {
                        pos.pos[0] = pos.pos[1] = 0;
                        MWBase::Environment::get().getWorld()->changeToInteriorCell (cell, pos);
                    }
                }
        };

        class OpCOE : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Integer x = runtime[0].mInteger;
                    runtime.pop();

                    Interpreter::Type_Integer y = runtime[0].mInteger;
                    runtime.pop();

                    ESM::Position pos;

                    MWBase::Environment::get().getWorld()->indexToPosition (x, y, pos.pos[0], pos.pos[1], true);
                    pos.pos[2] = 0;

                    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

                    MWBase::Environment::get().getWorld()->changeToExteriorCell (pos);
                }
        };

        class OpGetInterior : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool interior =
                        MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell()->cell->mData.mFlags &
                            ESM::Cell::Interior;

                    runtime.push (interior ? 1 : 0);
                }
        };

        class OpGetPCCell : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    const ESM::Cell *cell = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell()->cell;

                    std::string current = cell->mName;

                    if (!(cell->mData.mFlags & ESM::Cell::Interior) && current.empty())
                    {
                        const ESM::Region *region =
                            MWBase::Environment::get().getWorld()->getStore().regions.find (cell->mRegion);

                        current = region->mName;
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
                    MWWorld::Ptr::CellStore *cell = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell();
                    runtime.push (cell->mWaterLevel);
                }
        };

        class OpSetWaterLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float level = runtime[0].mFloat;

                    MWWorld::Ptr::CellStore *cell = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell();

                    if (!(cell->cell->mData.mFlags & ESM::Cell::Interior))
                        throw std::runtime_error("Can't set water level in exterior cell");

                    cell->mWaterLevel = level;
                    MWBase::Environment::get().getWorld()->setWaterHeight(cell->mWaterLevel);
                }
        };

        class OpModWaterLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float level = runtime[0].mFloat;

                    MWWorld::Ptr::CellStore *cell = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell();

                    if (!(cell->cell->mData.mFlags & ESM::Cell::Interior))
                        throw std::runtime_error("Can't set water level in exterior cell");

                    cell->mWaterLevel +=level;
                    MWBase::Environment::get().getWorld()->setWaterHeight(cell->mWaterLevel);
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

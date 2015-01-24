
#include "cellextensions.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/cellstore.hpp"

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
                    MWBase::World *world = MWBase::Environment::get().getWorld();

                    world->getPlayer().setTeleported(true);
                    if (world->findExteriorPosition(cell, pos))
                    {
                        world->changeToExteriorCell(pos);
                        world->fixPosition(world->getPlayerPtr());
                    }
                    else
                    {
                        // Change to interior even if findInteriorPosition()
                        // yields false. In this case position will be zero-point.
                        world->findInteriorPosition(cell, pos);
                        world->changeToInteriorCell(cell, pos);
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
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    world->getPlayer().setTeleported(true);
                    world->indexToPosition (x, y, pos.pos[0], pos.pos[1], true);
                    pos.pos[2] = 0;

                    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

                    world->changeToExteriorCell (pos);
                    world->fixPosition(world->getPlayerPtr());
                }
        };

        class OpGetInterior : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    if (!MWBase::Environment::get().getWorld()->getPlayerPtr().isInCell())
                    {
                        runtime.push (0);
                        return;
                    }

                    bool interior =
                        !MWBase::Environment::get().getWorld()->getPlayerPtr().getCell()->getCell()->isExterior();

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

                    if (!MWBase::Environment::get().getWorld()->getPlayerPtr().isInCell())
                    {
                        runtime.push(0);
                        return;
                    }
                    const MWWorld::CellStore *cell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell();

                    std::string current = MWBase::Environment::get().getWorld()->getCellName(cell);
                    Misc::StringUtils::toLower(current);

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
                    if (!MWBase::Environment::get().getWorld()->getPlayerPtr().isInCell())
                    {
                        runtime.push(0.f);
                        return;
                    }
                    MWWorld::CellStore *cell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell();
                    if (cell->getCell()->hasWater())
                        runtime.push (cell->getWaterLevel());
                    else
                        runtime.push (-std::numeric_limits<float>::max());
                }
        };

        class OpSetWaterLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float level = runtime[0].mFloat;

                    if (!MWBase::Environment::get().getWorld()->getPlayerPtr().isInCell())
                    {
                        return;
                    }

                    MWWorld::CellStore *cell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell();

                    if (cell->getCell()->isExterior())
                        throw std::runtime_error("Can't set water level in exterior cell");

                    cell->setWaterLevel (level);
                    MWBase::Environment::get().getWorld()->setWaterHeight (cell->getWaterLevel());
                }
        };

        class OpModWaterLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float level = runtime[0].mFloat;

                    if (!MWBase::Environment::get().getWorld()->getPlayerPtr().isInCell())
                    {
                        return;
                    }

                    MWWorld::CellStore *cell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell();

                    if (cell->getCell()->isExterior())
                        throw std::runtime_error("Can't set water level in exterior cell");

                    cell->setWaterLevel (cell->getWaterLevel()+level);
                    MWBase::Environment::get().getWorld()->setWaterHeight(cell->getWaterLevel());
                }
        };


        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Cell::opcodeCellChanged, new OpCellChanged);
            interpreter.installSegment5 (Compiler::Cell::opcodeCOC, new OpCOC);
            interpreter.installSegment5 (Compiler::Cell::opcodeCOE, new OpCOE);
            interpreter.installSegment5 (Compiler::Cell::opcodeGetInterior, new OpGetInterior);
            interpreter.installSegment5 (Compiler::Cell::opcodeGetPCCell, new OpGetPCCell);
            interpreter.installSegment5 (Compiler::Cell::opcodeGetWaterLevel, new OpGetWaterLevel);
            interpreter.installSegment5 (Compiler::Cell::opcodeSetWaterLevel, new OpSetWaterLevel);
            interpreter.installSegment5 (Compiler::Cell::opcodeModWaterLevel, new OpModWaterLevel);
        }
    }
}

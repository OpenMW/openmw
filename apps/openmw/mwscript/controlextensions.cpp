#include "controlextensions.hpp"

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Control
    {
        class OpSetControl : public Interpreter::Opcode0
        {
            std::string_view mControl;
            bool mEnable;

        public:
            OpSetControl(std::string_view control, bool enable)
                : mControl(control)
                , mEnable(enable)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getInputManager()->toggleControlSwitch(mControl, mEnable);
            }
        };

        class OpGetDisabled : public Interpreter::Opcode0
        {
            std::string_view mControl;

        public:
            OpGetDisabled(std::string_view control)
                : mControl(control)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(!MWBase::Environment::get().getInputManager()->getControlSwitch(mControl));
            }
        };

        class OpToggleCollision : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleCollisionMode();

                runtime.getContext().report(enabled ? "Collision -> On" : "Collision -> Off");
            }
        };

        template <class R>
        class OpClearMovementFlag : public Interpreter::Opcode0
        {
            MWMechanics::CreatureStats::Flag mFlag;

        public:
            OpClearMovementFlag(MWMechanics::CreatureStats::Flag flag)
                : mFlag(flag)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ptr.getClass().getCreatureStats(ptr).setMovementFlag(mFlag, false);
            }
        };

        template <class R>
        class OpSetMovementFlag : public Interpreter::Opcode0
        {
            MWMechanics::CreatureStats::Flag mFlag;

        public:
            OpSetMovementFlag(MWMechanics::CreatureStats::Flag flag)
                : mFlag(flag)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ptr.getClass().getCreatureStats(ptr).setMovementFlag(mFlag, true);
            }
        };

        template <class R>
        class OpGetForceRun : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                runtime.push(stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceRun));
            }
        };

        template <class R>
        class OpGetForceJump : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                runtime.push(stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceJump));
            }
        };

        template <class R>
        class OpGetForceMoveJump : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                runtime.push(stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceMoveJump));
            }
        };

        template <class R>
        class OpGetForceSneak : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                runtime.push(stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceSneak));
            }
        };

        class OpGetPcRunning : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                runtime.push(MWBase::Environment::get().getMechanicsManager()->isRunning(ptr));
            }
        };

        class OpGetPcSneaking : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                runtime.push(MWBase::Environment::get().getMechanicsManager()->isSneaking(ptr));
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            for (int i = 0; i < Compiler::Control::numberOfControls; ++i)
            {
                interpreter.installSegment5<OpSetControl>(
                    Compiler::Control::opcodeEnable + i, Compiler::Control::controls[i], true);
                interpreter.installSegment5<OpSetControl>(
                    Compiler::Control::opcodeDisable + i, Compiler::Control::controls[i], false);
                interpreter.installSegment5<OpGetDisabled>(
                    Compiler::Control::opcodeGetDisabled + i, Compiler::Control::controls[i]);
            }

            interpreter.installSegment5<OpToggleCollision>(Compiler::Control::opcodeToggleCollision);

            // Force Run
            interpreter.installSegment5<OpClearMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeClearForceRun, MWMechanics::CreatureStats::Flag_ForceRun);
            interpreter.installSegment5<OpClearMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeClearForceRunExplicit, MWMechanics::CreatureStats::Flag_ForceRun);
            interpreter.installSegment5<OpSetMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeForceRun, MWMechanics::CreatureStats::Flag_ForceRun);
            interpreter.installSegment5<OpSetMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeForceRunExplicit, MWMechanics::CreatureStats::Flag_ForceRun);

            // Force Jump
            interpreter.installSegment5<OpClearMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeClearForceJump, MWMechanics::CreatureStats::Flag_ForceJump);
            interpreter.installSegment5<OpClearMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeClearForceJumpExplicit, MWMechanics::CreatureStats::Flag_ForceJump);
            interpreter.installSegment5<OpSetMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeForceJump, MWMechanics::CreatureStats::Flag_ForceJump);
            interpreter.installSegment5<OpSetMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeForceJumpExplicit, MWMechanics::CreatureStats::Flag_ForceJump);

            // Force MoveJump
            interpreter.installSegment5<OpClearMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeClearForceMoveJump, MWMechanics::CreatureStats::Flag_ForceMoveJump);
            interpreter.installSegment5<OpClearMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeClearForceMoveJumpExplicit, MWMechanics::CreatureStats::Flag_ForceMoveJump);
            interpreter.installSegment5<OpSetMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeForceMoveJump, MWMechanics::CreatureStats::Flag_ForceMoveJump);
            interpreter.installSegment5<OpSetMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeForceMoveJumpExplicit, MWMechanics::CreatureStats::Flag_ForceMoveJump);

            // Force Sneak
            interpreter.installSegment5<OpClearMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeClearForceSneak, MWMechanics::CreatureStats::Flag_ForceSneak);
            interpreter.installSegment5<OpClearMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeClearForceSneakExplicit, MWMechanics::CreatureStats::Flag_ForceSneak);
            interpreter.installSegment5<OpSetMovementFlag<ImplicitRef>>(
                Compiler::Control::opcodeForceSneak, MWMechanics::CreatureStats::Flag_ForceSneak);
            interpreter.installSegment5<OpSetMovementFlag<ExplicitRef>>(
                Compiler::Control::opcodeForceSneakExplicit, MWMechanics::CreatureStats::Flag_ForceSneak);

            interpreter.installSegment5<OpGetPcRunning>(Compiler::Control::opcodeGetPcRunning);
            interpreter.installSegment5<OpGetPcSneaking>(Compiler::Control::opcodeGetPcSneaking);
            interpreter.installSegment5<OpGetForceRun<ImplicitRef>>(Compiler::Control::opcodeGetForceRun);
            interpreter.installSegment5<OpGetForceRun<ExplicitRef>>(Compiler::Control::opcodeGetForceRunExplicit);
            interpreter.installSegment5<OpGetForceJump<ImplicitRef>>(Compiler::Control::opcodeGetForceJump);
            interpreter.installSegment5<OpGetForceJump<ExplicitRef>>(Compiler::Control::opcodeGetForceJumpExplicit);
            interpreter.installSegment5<OpGetForceMoveJump<ImplicitRef>>(Compiler::Control::opcodeGetForceMoveJump);
            interpreter.installSegment5<OpGetForceMoveJump<ExplicitRef>>(
                Compiler::Control::opcodeGetForceMoveJumpExplicit);
            interpreter.installSegment5<OpGetForceSneak<ImplicitRef>>(Compiler::Control::opcodeGetForceSneak);
            interpreter.installSegment5<OpGetForceSneak<ExplicitRef>>(Compiler::Control::opcodeGetForceSneakExplicit);
        }
    }
}

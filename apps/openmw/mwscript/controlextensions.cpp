#include "controlextensions.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Control
    {
        class OpSetControl : public Interpreter::Opcode0
        {
                std::string mControl;
                bool mEnable;

            public:

                OpSetControl (const std::string& control, bool enable)
                : mControl (control), mEnable (enable)
                {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get()
                        .getInputManager()
                        ->toggleControlSwitch(mControl, mEnable);
                }
        };

        class OpGetDisabled : public Interpreter::Opcode0
        {
                std::string mControl;

            public:

                OpGetDisabled (const std::string& control)
                : mControl (control)
                {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push(!MWBase::Environment::get().getInputManager()->getControlSwitch (mControl));
                }
        };

        class OpToggleCollision : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled = MWBase::Environment::get().getWorld()->toggleCollisionMode();

                    runtime.getContext().report (enabled ? "Collision -> On" : "Collision -> Off");
                }
        };

        template<class R>
        class OpClearMovementFlag : public Interpreter::Opcode0
        {
                MWMechanics::CreatureStats::Flag mFlag;

            public:

                OpClearMovementFlag (MWMechanics::CreatureStats::Flag flag) : mFlag (flag) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    ptr.getClass().getCreatureStats(ptr).setMovementFlag (mFlag, false);
                }
        };

        template<class R>
        class OpSetMovementFlag : public Interpreter::Opcode0
        {
                MWMechanics::CreatureStats::Flag mFlag;

            public:

                OpSetMovementFlag (MWMechanics::CreatureStats::Flag flag) : mFlag (flag) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    ptr.getClass().getCreatureStats(ptr).setMovementFlag (mFlag, true);
                }
        };

        template <class R>
        class OpGetForceRun : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);
                    runtime.push (stats.getMovementFlag (MWMechanics::CreatureStats::Flag_ForceRun));
                }
        };

        template <class R>
        class OpGetForceJump : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);
                    runtime.push (stats.getMovementFlag (MWMechanics::CreatureStats::Flag_ForceJump));
                }
        };

        template <class R>
        class OpGetForceMoveJump : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);
                    runtime.push (stats.getMovementFlag (MWMechanics::CreatureStats::Flag_ForceMoveJump));
                }
        };

        template <class R>
        class OpGetForceSneak : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                    runtime.push (stats.getMovementFlag (MWMechanics::CreatureStats::Flag_ForceSneak));
                }
        };

        class OpGetPcRunning : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                    MWBase::World* world = MWBase::Environment::get().getWorld();

                    bool stanceOn = stats.getStance(MWMechanics::CreatureStats::Stance_Run);
                    bool running = MWBase::Environment::get().getMechanicsManager()->isRunning(ptr);
                    bool inair = !world->isOnGround(ptr) && !world->isSwimming(ptr) && !world->isFlying(ptr);

                    runtime.push(stanceOn && (running || inair));
                }
        };

        class OpGetPcSneaking : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                    runtime.push(MWBase::Environment::get().getMechanicsManager()->isSneaking(ptr));
                }
        };


        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            for (int i=0; i<Compiler::Control::numberOfControls; ++i)
            {
                interpreter.installSegment5 (Compiler::Control::opcodeEnable+i, new OpSetControl (Compiler::Control::controls[i], true));
                interpreter.installSegment5 (Compiler::Control::opcodeDisable+i, new OpSetControl (Compiler::Control::controls[i], false));
                interpreter.installSegment5 (Compiler::Control::opcodeGetDisabled+i, new OpGetDisabled (Compiler::Control::controls[i]));
            }

            interpreter.installSegment5 (Compiler::Control::opcodeToggleCollision, new OpToggleCollision);

            //Force Run
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceRun,
                new OpClearMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceRun));
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceRunExplicit,
                new OpClearMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceRun));
            interpreter.installSegment5 (Compiler::Control::opcodeForceRun,
                new OpSetMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceRun));
            interpreter.installSegment5 (Compiler::Control::opcodeForceRunExplicit,
                new OpSetMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceRun));

            //Force Jump
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceJump,
                new OpClearMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceJump));
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceJumpExplicit,
                new OpClearMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceJump));
            interpreter.installSegment5 (Compiler::Control::opcodeForceJump,
                new OpSetMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceJump));
            interpreter.installSegment5 (Compiler::Control::opcodeForceJumpExplicit,
                new OpSetMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceJump));

            //Force MoveJump
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceMoveJump,
                new OpClearMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceMoveJump));
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceMoveJumpExplicit,
                new OpClearMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceMoveJump));
            interpreter.installSegment5 (Compiler::Control::opcodeForceMoveJump,
                new OpSetMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceMoveJump));
            interpreter.installSegment5 (Compiler::Control::opcodeForceMoveJumpExplicit,
                new OpSetMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceMoveJump));

            //Force Sneak
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceSneak,
                new OpClearMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceSneak));
            interpreter.installSegment5 (Compiler::Control::opcodeClearForceSneakExplicit,
                new OpClearMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceSneak));
            interpreter.installSegment5 (Compiler::Control::opcodeForceSneak,
                new OpSetMovementFlag<ImplicitRef> (MWMechanics::CreatureStats::Flag_ForceSneak));
            interpreter.installSegment5 (Compiler::Control::opcodeForceSneakExplicit,
                new OpSetMovementFlag<ExplicitRef> (MWMechanics::CreatureStats::Flag_ForceSneak));

            interpreter.installSegment5 (Compiler::Control::opcodeGetPcRunning, new OpGetPcRunning);
            interpreter.installSegment5 (Compiler::Control::opcodeGetPcSneaking, new OpGetPcSneaking);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceRun, new OpGetForceRun<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceRunExplicit, new OpGetForceRun<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceJump, new OpGetForceJump<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceJumpExplicit, new OpGetForceJump<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceMoveJump, new OpGetForceMoveJump<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceMoveJumpExplicit, new OpGetForceMoveJump<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceSneak, new OpGetForceSneak<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Control::opcodeGetForceSneakExplicit, new OpGetForceSneak<ExplicitRef>);
        }
    }
}

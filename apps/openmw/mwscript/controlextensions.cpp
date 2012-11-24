
#include "controlextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

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
                    InterpreterContext& context
                        = static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled = MWBase::Environment::get().getWorld()->toggleCollisionMode();

                    context.report (enabled ? "Collision -> On" : "Collision -> Off");
                }
        };

        template<class R>
        class OpClearMovementFlag : public Interpreter::Opcode0
        {
                MWMechanics::NpcStats::Flag mFlag;

            public:

                OpClearMovementFlag (MWMechanics::NpcStats::Flag flag) : mFlag (flag) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).getNpcStats (ptr).setMovementFlag (mFlag, false);
                }
        };

        template<class R>
        class OpSetMovementFlag : public Interpreter::Opcode0
        {
                MWMechanics::NpcStats::Flag mFlag;

            public:

                OpSetMovementFlag (MWMechanics::NpcStats::Flag flag) : mFlag (flag) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).getNpcStats (ptr).setMovementFlag (mFlag, true);
                }
        };

        template <class R>
        class OpGetForceRun : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWMechanics::NpcStats& npcStats = MWWorld::Class::get(ptr).getNpcStats (ptr);

                    runtime.push (npcStats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceRun));
                }
        };

        template <class R>
        class OpGetForceSneak : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWMechanics::NpcStats& npcStats = MWWorld::Class::get(ptr).getNpcStats (ptr);

                    runtime.push (npcStats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceSneak));
                }
        };

        class OpGetPcRunning : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer();
                    runtime.push (MWWorld::Class::get(ptr).getStance (ptr, MWWorld::Class::Run));
                }
        };

        class OpGetPcSneaking : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer();
                    runtime.push (MWWorld::Class::get(ptr).getStance (ptr, MWWorld::Class::Sneak));
                }
        };

        const int numberOfControls = 7;

        const int opcodeEnable = 0x200007e;
        const int opcodeDisable = 0x2000085;
        const int opcodeToggleCollision = 0x2000130;
        const int opcodeClearForceRun = 0x2000154;
        const int opcodeClearForceRunExplicit = 0x2000155;
        const int opcodeForceRun = 0x2000156;
        const int opcodeForceRunExplicit = 0x2000157;
        const int opcodeClearForceSneak = 0x2000158;
        const int opcodeClearForceSneakExplicit = 0x2000159;
        const int opcodeForceSneak = 0x200015a;
        const int opcodeForceSneakExplicit = 0x200015b;
        const int opcodeGetDisabled = 0x2000175;
        const int opcodeGetPcRunning = 0x20001c9;
        const int opcodeGetPcSneaking = 0x20001ca;
        const int opcodeGetForceRun = 0x20001cb;
        const int opcodeGetForceSneak = 0x20001cc;
        const int opcodeGetForceRunExplicit = 0x20001cd;
        const int opcodeGetForceSneakExplicit = 0x20001ce;

        const char *controls[numberOfControls] =
        {
            "playercontrols", "playerfighting", "playerjumping", "playerlooking", "playermagic",
            "playerviewswitch", "vanitymode"
        };

        void registerExtensions (Compiler::Extensions& extensions)
        {
            std::string enable ("enable");
            std::string disable ("disable");

            for (int i=0; i<numberOfControls; ++i)
            {
                extensions.registerInstruction (enable + controls[i], "", opcodeEnable+i);
                extensions.registerInstruction (disable + controls[i], "", opcodeDisable+i);
                extensions.registerFunction (std::string("get") + controls[i] + std::string("disabled"), 'l', "", opcodeGetDisabled+i);
            }

            extensions.registerInstruction ("togglecollision", "", opcodeToggleCollision);
            extensions.registerInstruction ("tcl", "", opcodeToggleCollision);

            extensions.registerInstruction ("clearforcerun", "", opcodeClearForceRun,
                opcodeClearForceRunExplicit);
            extensions.registerInstruction ("forcerun", "", opcodeForceRun,
                opcodeForceRunExplicit);

            extensions.registerInstruction ("clearforcesneak", "", opcodeClearForceSneak,
                opcodeClearForceSneakExplicit);
            extensions.registerInstruction ("forcesneak", "", opcodeForceSneak,
                opcodeForceSneakExplicit);
            extensions.registerFunction ("getpcrunning", 'l', "", opcodeGetPcRunning);
            extensions.registerFunction ("getpcsneaking", 'l', "", opcodeGetPcSneaking);
            extensions.registerFunction ("getforcerun", 'l', "", opcodeGetForceRun, opcodeGetForceRunExplicit);
            extensions.registerFunction ("getforcesneak", 'l', "", opcodeGetForceSneak, opcodeGetForceSneakExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            for (int i=0; i<numberOfControls; ++i)
            {
                interpreter.installSegment5 (opcodeEnable+i, new OpSetControl (controls[i], true));
                interpreter.installSegment5 (opcodeDisable+i, new OpSetControl (controls[i], false));
                interpreter.installSegment5 (opcodeGetDisabled+i, new OpGetDisabled (controls[i]));
            }

            interpreter.installSegment5 (opcodeToggleCollision, new OpToggleCollision);

            interpreter.installSegment5 (opcodeClearForceRun,
                new OpClearMovementFlag<ImplicitRef> (MWMechanics::NpcStats::Flag_ForceRun));
            interpreter.installSegment5 (opcodeForceRun,
                new OpSetMovementFlag<ImplicitRef> (MWMechanics::NpcStats::Flag_ForceRun));
            interpreter.installSegment5 (opcodeClearForceSneak,
                new OpClearMovementFlag<ImplicitRef> (MWMechanics::NpcStats::Flag_ForceSneak));
            interpreter.installSegment5 (opcodeForceSneak,
                new OpSetMovementFlag<ImplicitRef> (MWMechanics::NpcStats::Flag_ForceSneak));

            interpreter.installSegment5 (opcodeClearForceRunExplicit,
                new OpClearMovementFlag<ExplicitRef> (MWMechanics::NpcStats::Flag_ForceRun));
            interpreter.installSegment5 (opcodeForceRunExplicit,
                new OpSetMovementFlag<ExplicitRef> (MWMechanics::NpcStats::Flag_ForceRun));
            interpreter.installSegment5 (opcodeClearForceSneakExplicit,
                new OpClearMovementFlag<ExplicitRef> (MWMechanics::NpcStats::Flag_ForceSneak));
            interpreter.installSegment5 (opcodeForceSneakExplicit,
                new OpSetMovementFlag<ExplicitRef> (MWMechanics::NpcStats::Flag_ForceSneak));
            interpreter.installSegment5 (opcodeGetPcRunning, new OpGetPcRunning);
            interpreter.installSegment5 (opcodeGetPcSneaking, new OpGetPcSneaking);
            interpreter.installSegment5 (opcodeGetForceRun, new OpGetForceRun<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetForceRunExplicit, new OpGetForceRun<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetForceSneak, new OpGetForceSneak<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetForceSneakExplicit, new OpGetForceSneak<ExplicitRef>);
        }
    }
}

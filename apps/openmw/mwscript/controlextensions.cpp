
#include "controlextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

#include <iostream>

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
                    if (mEnable)
                        std::cout << "enable: " << mControl << std::endl;
                    else
                        std::cout << "disable: " << mControl << std::endl;
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
        class OpClearForceRun : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mForceRun = false;
                }
        };

        template<class R>
        class OpForceRun : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mForceRun = true;
                }
        };

        template<class R>
        class OpClearForceSneak : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mForceSneak = false;
                }
        };

        template<class R>
        class OpForceSneak : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mForceSneak = true;
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
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            for (int i=0; i<numberOfControls; ++i)
            {
                interpreter.installSegment5 (opcodeEnable+i, new OpSetControl (controls[i], true));
                interpreter.installSegment5 (opcodeDisable+i, new OpSetControl (controls[i], false));
            }

            interpreter.installSegment5 (opcodeToggleCollision, new OpToggleCollision);

            interpreter.installSegment5 (opcodeClearForceRun, new OpClearForceRun<ImplicitRef>);
            interpreter.installSegment5 (opcodeForceRun, new OpForceRun<ImplicitRef>);
            interpreter.installSegment5 (opcodeClearForceSneak, new OpClearForceSneak<ImplicitRef>);
            interpreter.installSegment5 (opcodeForceSneak, new OpForceSneak<ImplicitRef>);

            interpreter.installSegment5 (opcodeClearForceRunExplicit,
                new OpClearForceRun<ExplicitRef>);
            interpreter.installSegment5 (opcodeForceRunExplicit,
                new OpForceRun<ExplicitRef>);
            interpreter.installSegment5 (opcodeClearForceSneakExplicit,
                new OpClearForceSneak<ExplicitRef>);
            interpreter.installSegment5 (opcodeForceSneakExplicit,
                new OpForceSneak<ExplicitRef>);
        }
    }
}


#include "aiextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/class.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

#include <iostream>

namespace MWScript
{
    namespace Ai
    {
        template<class R>
        class OpAiTravel : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    std::cout << "AiTravel: " << x << ", " << y << ", " << z << std::endl;
                }
        };

        template<class R>
        class OpAiEscort : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string actor = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float duration = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    std::cout << "AiEscort: " << x << ", " << y << ", " << z << ", " << duration
                        << std::endl;
                }
        };

        template<class R>
        class OpGetAiPackageDone : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = 0;

                    runtime.push (value);
                }
        };

        template<class R>
        class OpAiWander : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

//                    Interpreter::Type_Float range = runtime[0].mFloat;
                    runtime.pop();

//                    Interpreter::Type_Float duration = runtime[0].mFloat;
                    runtime.pop();

//                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional idle arguments for now
                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    std::cout << "AiWanter" << std::endl;
                }
        };

        template<class R>
        class OpSetHello : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setHello(value);
                }
        };

        template<class R>
        class OpSetFight : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setFight(value);
                }
        };

        template<class R>
        class OpSetFlee : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setFlee(value);
                }
        };

        template<class R>
        class OpSetAlarm : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setAlarm(value);
                }
        };


        const int opcodeAiTravel = 0x20000;
        const int opcodeAiTravelExplicit = 0x20001;
        const int opcodeAiEscort = 0x20002;
        const int opcodeAiEscortExplicit = 0x20003;
        const int opcodeGetAiPackageDone = 0x200007c;
        const int opcodeGetAiPackageDoneExplicit = 0x200007d;
        const int opcodeAiWander = 0x20010;
        const int opcodeAiWanderExplicit = 0x20011;
        const int opcodeSetHello = 0x200015e;
        const int opcodeSetHelloExplicit = 0x200015d;
        const int opcodeSetFight = 0x200015e;
        const int opcodeSetFightExplicit = 0x200015f;
        const int opcodeSetFlee = 0x2000160;
        const int opcodeSetFleeExplicit = 0x2000161;
        const int opcodeSetAlarm = 0x2000162;
        const int opcodeSetAlarmExplicit = 0x2000163;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("aitravel", "fff/l", opcodeAiTravel,
                opcodeAiTravelExplicit);
            extensions.registerInstruction ("aiescort", "cffff/l", opcodeAiEscort,
                opcodeAiEscortExplicit);
            extensions.registerInstruction ("aiwander", "fff/llllllllll", opcodeAiWander,
                opcodeAiWanderExplicit);

            extensions.registerFunction ("getaipackagedone", 'l', "", opcodeGetAiPackageDone,
                opcodeGetAiPackageDoneExplicit);

            extensions.registerInstruction ("sethello", "l", opcodeSetHello, opcodeSetHelloExplicit);
            extensions.registerInstruction ("setfight", "l", opcodeSetFight, opcodeSetFightExplicit);
            extensions.registerInstruction ("setflee", "l", opcodeSetFlee, opcodeSetFleeExplicit);
            extensions.registerInstruction ("setalarm", "l", opcodeSetAlarm, opcodeSetAlarmExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment3 (opcodeAiTravel, new OpAiTravel<ImplicitRef>);
            interpreter.installSegment3 (opcodeAiTravelExplicit, new OpAiTravel<ExplicitRef>);
            interpreter.installSegment3 (opcodeAiEscort, new OpAiEscort<ImplicitRef>);
            interpreter.installSegment3 (opcodeAiEscortExplicit, new OpAiEscort<ExplicitRef>);
            interpreter.installSegment3 (opcodeAiWander, new OpAiWander<ImplicitRef>);
            interpreter.installSegment3 (opcodeAiWanderExplicit, new OpAiWander<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetAiPackageDone, new OpGetAiPackageDone<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetAiPackageDoneExplicit,
                new OpGetAiPackageDone<ExplicitRef>);
            interpreter.installSegment5 (opcodeSetHello, new OpSetHello<ImplicitRef>);
            interpreter.installSegment5 (opcodeSetHelloExplicit, new OpSetHello<ExplicitRef>);
            interpreter.installSegment5 (opcodeSetFight, new OpSetFight<ImplicitRef>);
            interpreter.installSegment5 (opcodeSetFightExplicit, new OpSetFight<ExplicitRef>);
            interpreter.installSegment5 (opcodeSetFlee, new OpSetFlee<ImplicitRef>);
            interpreter.installSegment5 (opcodeSetFleeExplicit, new OpSetFlee<ExplicitRef>);
            interpreter.installSegment5 (opcodeSetAlarm, new OpSetAlarm<ImplicitRef>);
            interpreter.installSegment5 (opcodeSetAlarmExplicit, new OpSetAlarm<ExplicitRef>);
        }
    }
}

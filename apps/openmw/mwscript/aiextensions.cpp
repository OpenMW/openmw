
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

                    std::cout << "AiWander" << std::endl;
                }
        };

        template<class R>
        class OpGetAiSetting : public Interpreter::Opcode0
        {
            int mIndex;
            public:
                OpGetAiSetting(int index) : mIndex(index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push(MWWorld::Class::get (ptr).getCreatureStats (ptr).getAiSetting (mIndex));
                }
        };
        template<class R>
        class OpModAiSetting : public Interpreter::Opcode0
        {
            int mIndex;
            public:
                OpModAiSetting(int index) : mIndex(index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setAiSetting (mIndex,
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).getAiSetting (mIndex) + value);
                }
        };
        template<class R>
        class OpSetAiSetting : public Interpreter::Opcode0
        {
            int mIndex;
            public:
                OpSetAiSetting(int index) : mIndex(index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setAiSetting (mIndex,
                        value);
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
        const int opcodeModHello = 0x20001b7;
        const int opcodeModHelloExplicit = 0x20001b8;
        const int opcodeModFight = 0x20001b9;
        const int opcodeModFightExplicit = 0x20001ba;
        const int opcodeModFlee = 0x20001bb;
        const int opcodeModFleeExplicit = 0x20001bc;
        const int opcodeModAlarm = 0x20001bd;
        const int opcodeModAlarmExplicit = 0x20001be;
        const int opcodeGetHello = 0x20001bf;
        const int opcodeGetHelloExplicit = 0x20001c0;
        const int opcodeGetFight = 0x20001c1;
        const int opcodeGetFightExplicit = 0x20001c2;
        const int opcodeGetFlee = 0x20001c3;
        const int opcodeGetFleeExplicit = 0x20001c4;
        const int opcodeGetAlarm = 0x20001c5;
        const int opcodeGetAlarmExplicit = 0x20001c6;

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
            extensions.registerInstruction ("modhello", "l", opcodeModHello, opcodeModHelloExplicit);
            extensions.registerInstruction ("modfight", "l", opcodeModFight, opcodeModFightExplicit);
            extensions.registerInstruction ("modflee", "l", opcodeModFlee, opcodeModFleeExplicit);
            extensions.registerInstruction ("modalarm", "l", opcodeModAlarm, opcodeModAlarmExplicit);
            extensions.registerFunction ("gethello", 'l', "", opcodeGetHello, opcodeGetHelloExplicit);
            extensions.registerFunction ("getfight", 'l', "", opcodeGetFight, opcodeGetFightExplicit);
            extensions.registerFunction ("getflee", 'l', "", opcodeGetFlee, opcodeGetFleeExplicit);
            extensions.registerFunction ("getalarm", 'l', "", opcodeGetAlarm, opcodeGetAlarmExplicit);
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
            interpreter.installSegment5 (opcodeSetHello, new OpSetAiSetting<ImplicitRef>(0));
            interpreter.installSegment5 (opcodeSetHelloExplicit, new OpSetAiSetting<ExplicitRef>(0));
            interpreter.installSegment5 (opcodeSetFight, new OpSetAiSetting<ImplicitRef>(1));
            interpreter.installSegment5 (opcodeSetFightExplicit, new OpSetAiSetting<ExplicitRef>(1));
            interpreter.installSegment5 (opcodeSetFlee, new OpSetAiSetting<ImplicitRef>(2));
            interpreter.installSegment5 (opcodeSetFleeExplicit, new OpSetAiSetting<ExplicitRef>(2));
            interpreter.installSegment5 (opcodeSetAlarm, new OpSetAiSetting<ImplicitRef>(3));
            interpreter.installSegment5 (opcodeSetAlarmExplicit, new OpSetAiSetting<ExplicitRef>(3));

            interpreter.installSegment5 (opcodeModHello, new OpModAiSetting<ImplicitRef>(0));
            interpreter.installSegment5 (opcodeModHelloExplicit, new OpModAiSetting<ExplicitRef>(0));
            interpreter.installSegment5 (opcodeModFight, new OpModAiSetting<ImplicitRef>(1));
            interpreter.installSegment5 (opcodeModFightExplicit, new OpModAiSetting<ExplicitRef>(1));
            interpreter.installSegment5 (opcodeModFlee, new OpModAiSetting<ImplicitRef>(2));
            interpreter.installSegment5 (opcodeModFleeExplicit, new OpModAiSetting<ExplicitRef>(2));
            interpreter.installSegment5 (opcodeModAlarm, new OpModAiSetting<ImplicitRef>(3));
            interpreter.installSegment5 (opcodeModAlarmExplicit, new OpModAiSetting<ExplicitRef>(3));

            interpreter.installSegment5 (opcodeGetHello, new OpGetAiSetting<ImplicitRef>(0));
            interpreter.installSegment5 (opcodeGetHelloExplicit, new OpGetAiSetting<ExplicitRef>(0));
            interpreter.installSegment5 (opcodeGetFight, new OpGetAiSetting<ImplicitRef>(1));
            interpreter.installSegment5 (opcodeGetFightExplicit, new OpGetAiSetting<ExplicitRef>(1));
            interpreter.installSegment5 (opcodeGetFlee, new OpGetAiSetting<ImplicitRef>(2));
            interpreter.installSegment5 (opcodeGetFleeExplicit, new OpGetAiSetting<ExplicitRef>(2));
            interpreter.installSegment5 (opcodeGetAlarm, new OpGetAiSetting<ImplicitRef>(3));
            interpreter.installSegment5 (opcodeGetAlarmExplicit, new OpGetAiSetting<ExplicitRef>(3));
        }
    }
}

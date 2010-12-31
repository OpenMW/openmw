
#include "aiextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

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

                    Interpreter::Type_Float x = runtime[0].mInteger;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mInteger;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mInteger;
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

                    Interpreter::Type_Float duration = runtime[0].mInteger;
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mInteger;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mInteger;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mInteger;
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



        const int opcodeAiTravel = 0x20000;
        const int opcodeAiTravelExplicit = 0x20001;
        const int opcodeAiEscort = 0x20002;
        const int opcodeAiEscortExplicit = 0x20003;
        const int opcodeGetAiPackageDone = 0x200007c;
        const int opcodeGetAiPackageDoneExplicit = 0x200007d;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("aitravel", "lll/l", opcodeAiTravel,
                opcodeAiTravelExplicit);
            extensions.registerInstruction ("aiescort", "cllll/l", opcodeAiEscort,
                opcodeAiEscortExplicit);

            extensions.registerFunction ("getaipackagedone", 'l', "", opcodeGetAiPackageDone,
                opcodeGetAiPackageDoneExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment3 (opcodeAiTravel, new OpAiTravel<ImplicitRef>);
            interpreter.installSegment3 (opcodeAiTravelExplicit, new OpAiTravel<ExplicitRef>);
            interpreter.installSegment3 (opcodeAiEscort, new OpAiEscort<ImplicitRef>);
            interpreter.installSegment3 (opcodeAiEscortExplicit, new OpAiEscort<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetAiPackageDone, new OpGetAiPackageDone<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetAiPackageDoneExplicit,
                new OpGetAiPackageDone<ExplicitRef>);
        }
    }
}

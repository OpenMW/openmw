
#include "aiextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "interpretercontext.hpp"

#include <iostream>

namespace MWScript
{
    namespace Ai
    {
        class OpAiTravel : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

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

        class OpAiTravelExplicit : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

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

        class OpAiEscort : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

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

        class OpAiEscortExplicit : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

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

        class OpGetAiPackageDone : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    Interpreter::Type_Integer value = 0;

                    runtime.push (value);
                }
        };

        class OpGetAiPackageDoneExplicit : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

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
            interpreter.installSegment3 (opcodeAiTravel, new OpAiTravel);
            interpreter.installSegment3 (opcodeAiTravelExplicit, new OpAiTravelExplicit);
            interpreter.installSegment3 (opcodeAiEscort, new OpAiEscort);
            interpreter.installSegment3 (opcodeAiEscortExplicit, new OpAiEscortExplicit);
            interpreter.installSegment5 (opcodeGetAiPackageDone, new OpGetAiPackageDone);
            interpreter.installSegment5 (opcodeGetAiPackageDoneExplicit, new OpGetAiPackageDoneExplicit);
        }
    }
}

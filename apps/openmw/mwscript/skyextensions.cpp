
#include "skyextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Sky
    {
        class OpToggleSky : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled = MWBase::Environment::get().getWorld()->toggleSky();

                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    context.report (enabled ? "Sky -> On" : "Sky -> Off");
                }
        };

        class OpTurnMoonWhite : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWorld()->setMoonColour (false);
                }
        };

        class OpTurnMoonRed : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWorld()->setMoonColour (true);
                }
        };

        class OpGetMasserPhase : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (MWBase::Environment::get().getWorld()->getMasserPhase());
                }
        };

        class OpGetSecundaPhase : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (MWBase::Environment::get().getWorld()->getSecundaPhase());
                }
        };

        class OpGetCurrentWeather : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (MWBase::Environment::get().getWorld()->getCurrentWeather());
                }
        };

        class OpChangeWeather : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string region = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer id = runtime[0].mInteger;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->changeWeather(region, id);
                }
        };

        const int opcodeToggleSky = 0x2000021;
        const int opcodeTurnMoonWhite = 0x2000022;
        const int opcodeTurnMoonRed = 0x2000023;
        const int opcodeGetMasserPhase = 0x2000024;
        const int opcodeGetSecundaPhase = 0x2000025;
        const int opcodeGetCurrentWeather = 0x200013f;
        const int opcodeChangeWeather = 0x2000140;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("togglesky", "", opcodeToggleSky);
            extensions.registerInstruction ("ts", "", opcodeToggleSky);
            extensions.registerInstruction ("turnmoonwhite", "", opcodeTurnMoonWhite);
            extensions.registerInstruction ("turnmoonred", "", opcodeTurnMoonRed);
            extensions.registerInstruction ("changeweather", "Sl", opcodeChangeWeather);
            extensions.registerFunction ("getmasserphase", 'l', "", opcodeGetMasserPhase);
            extensions.registerFunction ("getsecundaphase", 'l', "", opcodeGetSecundaPhase);
            extensions.registerFunction ("getcurrentweather", 'l', "", opcodeGetCurrentWeather);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeToggleSky, new OpToggleSky);
            interpreter.installSegment5 (opcodeTurnMoonWhite, new OpTurnMoonWhite);
            interpreter.installSegment5 (opcodeTurnMoonRed, new OpTurnMoonRed);
            interpreter.installSegment5 (opcodeGetMasserPhase, new OpGetMasserPhase);
            interpreter.installSegment5 (opcodeGetSecundaPhase, new OpGetSecundaPhase);
            interpreter.installSegment5 (opcodeGetCurrentWeather, new OpGetCurrentWeather);
            interpreter.installSegment5 (opcodeChangeWeather, new OpChangeWeather);
        }
    }
}

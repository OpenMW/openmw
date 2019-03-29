#include "skyextensions.hpp"

#include <algorithm>

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

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

                    runtime.getContext().report (enabled ? "Sky -> On" : "Sky -> Off");
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

        class OpModRegion : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    std::string region = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::vector<char> chances;
                    chances.reserve(10);
                    while(arg0 > 0)
                    {
                        chances.push_back(std::max(0, std::min(127, runtime[0].mInteger)));
                        runtime.pop();
                        arg0--;
                    }

                    MWBase::Environment::get().getWorld()->modRegion(region, chances);
                }
        };


        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Sky::opcodeToggleSky, new OpToggleSky);
            interpreter.installSegment5 (Compiler::Sky::opcodeTurnMoonWhite, new OpTurnMoonWhite);
            interpreter.installSegment5 (Compiler::Sky::opcodeTurnMoonRed, new OpTurnMoonRed);
            interpreter.installSegment5 (Compiler::Sky::opcodeGetMasserPhase, new OpGetMasserPhase);
            interpreter.installSegment5 (Compiler::Sky::opcodeGetSecundaPhase, new OpGetSecundaPhase);
            interpreter.installSegment5 (Compiler::Sky::opcodeGetCurrentWeather, new OpGetCurrentWeather);
            interpreter.installSegment5 (Compiler::Sky::opcodeChangeWeather, new OpChangeWeather);
            interpreter.installSegment3 (Compiler::Sky::opcodeModRegion, new OpModRegion);
        }
    }
}

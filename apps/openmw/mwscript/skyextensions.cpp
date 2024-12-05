#include "skyextensions.hpp"

#include <algorithm>

#include <components/compiler/opcodes.hpp>

#include <components/esm3/loadregn.hpp>
#include <components/interpreter/context.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWScript
{
    namespace Sky
    {
        class OpToggleSky : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleSky();

                runtime.getContext().report(enabled ? "Sky -> On" : "Sky -> Off");
            }
        };

        class OpTurnMoonWhite : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getWorld()->setMoonColour(false);
            }
        };

        class OpTurnMoonRed : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getWorld()->setMoonColour(true);
            }
        };

        class OpGetMasserPhase : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWorld()->getMasserPhase());
            }
        };

        class OpGetSecundaPhase : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWorld()->getSecundaPhase());
            }
        };

        class OpGetCurrentWeather : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWorld()->getCurrentWeather());
            }
        };

        class OpChangeWeather : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId region = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Integer id = runtime[0].mInteger;
                runtime.pop();

                const ESM::Region* reg = MWBase::Environment::get().getESMStore()->get<ESM::Region>().search(region);
                if (reg)
                    MWBase::Environment::get().getWorld()->changeWeather(region, id);
                else
                    runtime.getContext().report("Warning: Region \"" + region.getRefIdString() + "\" was not found");
            }
        };

        class OpModRegion : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                std::string_view region{ runtime.getStringLiteral(runtime[0].mInteger) };
                runtime.pop();

                std::vector<uint8_t> chances;
                chances.reserve(10);
                while (arg0 > 0)
                {
                    chances.push_back(std::clamp(runtime[0].mInteger, 0, 100));
                    runtime.pop();
                    arg0--;
                }

                MWBase::Environment::get().getWorld()->modRegion(ESM::RefId::stringRefId(region), chances);
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5<OpToggleSky>(Compiler::Sky::opcodeToggleSky);
            interpreter.installSegment5<OpTurnMoonWhite>(Compiler::Sky::opcodeTurnMoonWhite);
            interpreter.installSegment5<OpTurnMoonRed>(Compiler::Sky::opcodeTurnMoonRed);
            interpreter.installSegment5<OpGetMasserPhase>(Compiler::Sky::opcodeGetMasserPhase);
            interpreter.installSegment5<OpGetSecundaPhase>(Compiler::Sky::opcodeGetSecundaPhase);
            interpreter.installSegment5<OpGetCurrentWeather>(Compiler::Sky::opcodeGetCurrentWeather);
            interpreter.installSegment5<OpChangeWeather>(Compiler::Sky::opcodeChangeWeather);
            interpreter.installSegment3<OpModRegion>(Compiler::Sky::opcodeModRegion);
        }
    }
}

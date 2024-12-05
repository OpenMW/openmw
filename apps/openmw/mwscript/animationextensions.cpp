#include "animationextensions.hpp"

#include <limits>
#include <stdexcept>

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "ref.hpp"

namespace MWScript
{
    namespace Animation
    {
        template <class R>
        class OpSkipAnim : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWBase::Environment::get().getMechanicsManager()->skipAnimation(ptr);
            }
        };

        template <class R>
        class OpPlayAnim : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getRefData().isEnabled())
                    return;

                std::string_view group = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                Interpreter::Type_Integer mode = 0;

                if (arg0 == 1)
                {
                    mode = runtime[0].mInteger;
                    runtime.pop();

                    if (mode < 0 || mode > 2)
                        throw std::runtime_error("animation mode out of range");
                }

                MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(
                    ptr, group, mode, std::numeric_limits<uint32_t>::max(), true);
            }
        };

        template <class R>
        class OpLoopAnim : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getRefData().isEnabled())
                    return;

                std::string_view group = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                Interpreter::Type_Integer loops = runtime[0].mInteger;
                runtime.pop();

                if (loops < 0)
                    throw std::runtime_error("number of animation loops must be non-negative");

                Interpreter::Type_Integer mode = 0;

                if (arg0 == 1)
                {
                    mode = runtime[0].mInteger;
                    runtime.pop();

                    if (mode < 0 || mode > 2)
                        throw std::runtime_error("animation mode out of range");
                }

                MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptr, group, mode, loops, true);
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5<OpSkipAnim<ImplicitRef>>(Compiler::Animation::opcodeSkipAnim);
            interpreter.installSegment5<OpSkipAnim<ExplicitRef>>(Compiler::Animation::opcodeSkipAnimExplicit);
            interpreter.installSegment3<OpPlayAnim<ImplicitRef>>(Compiler::Animation::opcodePlayAnim);
            interpreter.installSegment3<OpPlayAnim<ExplicitRef>>(Compiler::Animation::opcodePlayAnimExplicit);
            interpreter.installSegment3<OpLoopAnim<ImplicitRef>>(Compiler::Animation::opcodeLoopAnim);
            interpreter.installSegment3<OpLoopAnim<ExplicitRef>>(Compiler::Animation::opcodeLoopAnimExplicit);
        }
    }
}

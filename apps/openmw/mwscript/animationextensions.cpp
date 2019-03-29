#include "animationextensions.hpp"

#include <stdexcept>
#include <limits>

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Animation
    {
        template<class R>
        class OpSkipAnim : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWBase::Environment::get().getMechanicsManager()->skipAnimation (ptr);
               }
        };

        template<class R>
        class OpPlayAnim : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (!ptr.getRefData().isEnabled())
                        return;

                    std::string group = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer mode = 0;

                    if (arg0==1)
                    {
                        mode = runtime[0].mInteger;
                        runtime.pop();

                        if (mode<0 || mode>2)
                            throw std::runtime_error ("animation mode out of range");
                    }

                    MWBase::Environment::get().getMechanicsManager()->playAnimationGroup (ptr, group, mode, std::numeric_limits<int>::max(), true);
               }
        };

        template<class R>
        class OpLoopAnim : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (!ptr.getRefData().isEnabled())
                        return;

                    std::string group = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer loops = runtime[0].mInteger;
                    runtime.pop();

                    if (loops<0)
                        throw std::runtime_error ("number of animation loops must be non-negative");

                    Interpreter::Type_Integer mode = 0;

                    if (arg0==1)
                    {
                        mode = runtime[0].mInteger;
                        runtime.pop();

                        if (mode<0 || mode>2)
                            throw std::runtime_error ("animation mode out of range");
                    }

                    MWBase::Environment::get().getMechanicsManager()->playAnimationGroup (ptr, group, mode, loops + 1, true);
               }
        };
        

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Animation::opcodeSkipAnim, new OpSkipAnim<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Animation::opcodeSkipAnimExplicit, new OpSkipAnim<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Animation::opcodePlayAnim, new OpPlayAnim<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Animation::opcodePlayAnimExplicit, new OpPlayAnim<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Animation::opcodeLoopAnim, new OpLoopAnim<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Animation::opcodeLoopAnimExplicit, new OpLoopAnim<ExplicitRef>);
        }
    }
}

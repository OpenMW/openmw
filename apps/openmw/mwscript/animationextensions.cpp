
#include "animationextensions.hpp"

#include <stdexcept>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/world.hpp"

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

                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    context.getWorld().skipAnimation (ptr);
               }
        };

        template<class R>
        class OpPlayAnim : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

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

                    context.getWorld().playAnimationGroup (ptr, group, mode, 1);
               }
        };

        template<class R>
        class OpLoopAnim : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

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

                    context.getWorld().playAnimationGroup (ptr, group, mode, loops);
               }
        };

        const int opcodeSkipAnim = 0x2000138;
        const int opcodeSkipAnimExplicit = 0x2000139;
        const int opcodePlayAnim = 0x20006;
        const int opcodePlayAnimExplicit = 0x20007;
        const int opcodeLoopAnim = 0x20008;
        const int opcodeLoopAnimExplicit = 0x20009;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("skipanim", "", opcodeSkipAnim, opcodeSkipAnimExplicit);
            extensions.registerInstruction ("playgroup", "c/l", opcodePlayAnim, opcodePlayAnimExplicit);
            extensions.registerInstruction ("loopgroup", "cl/l", opcodeLoopAnim, opcodeLoopAnimExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeSkipAnim, new OpSkipAnim<ImplicitRef>);
            interpreter.installSegment5 (opcodeSkipAnimExplicit, new OpSkipAnim<ExplicitRef>);
            interpreter.installSegment3 (opcodePlayAnim, new OpPlayAnim<ImplicitRef>);
            interpreter.installSegment3 (opcodePlayAnimExplicit, new OpPlayAnim<ExplicitRef>);
            interpreter.installSegment3 (opcodeLoopAnim, new OpLoopAnim<ImplicitRef>);
            interpreter.installSegment3 (opcodeLoopAnimExplicit, new OpLoopAnim<ExplicitRef>);
        }
    }
}

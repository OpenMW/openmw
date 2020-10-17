#include "userextensions.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/context.hpp>

#include "ref.hpp"

namespace MWScript
{
    /// Temporary script extensions.
    ///
    /// \attention Do not commit changes to this file to a git repository!
    namespace User
    {
        class OpUser1 : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
                    runtime.getContext().report ("user1: not in use");
                }
        };

        class OpUser2 : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
                    runtime.getContext().report ("user2: not in use");
                }
        };

        template<class R>
        class OpUser3 : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
//                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.getContext().report ("user3: not in use");
                }
        };

        template<class R>
        class OpUser4 : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
//                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.getContext().report ("user4: not in use");
                }
        };
        

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::User::opcodeUser1, new OpUser1);
            interpreter.installSegment5 (Compiler::User::opcodeUser2, new OpUser2);
            interpreter.installSegment5 (Compiler::User::opcodeUser3, new OpUser3<ImplicitRef>);
            interpreter.installSegment5 (Compiler::User::opcodeUser3Explicit, new OpUser3<ExplicitRef>);
            interpreter.installSegment5 (Compiler::User::opcodeUser4, new OpUser4<ImplicitRef>);
            interpreter.installSegment5 (Compiler::User::opcodeUser4Explicit, new OpUser4<ExplicitRef>);
        }
    }
}

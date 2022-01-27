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
            interpreter.installSegment5<OpUser1>(Compiler::User::opcodeUser1);
            interpreter.installSegment5<OpUser2>(Compiler::User::opcodeUser2);
            interpreter.installSegment5<OpUser3<ImplicitRef>>(Compiler::User::opcodeUser3);
            interpreter.installSegment5<OpUser3<ExplicitRef>>(Compiler::User::opcodeUser3Explicit);
            interpreter.installSegment5<OpUser4<ImplicitRef>>(Compiler::User::opcodeUser4);
            interpreter.installSegment5<OpUser4<ExplicitRef>>(Compiler::User::opcodeUser4Explicit);
        }
    }
}


#include "userextensions.hpp"

#include <components/compiler/extensions.hpp>

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

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.getContext().report ("user1: not in use");
                }
        };

        class OpUser2 : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.getContext().report ("user2: not in use");
                }
        };

        template<class R>
        class OpUser3 : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
//                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.getContext().report ("user3: not in use");
                }
        };

        template<class R>
        class OpUser4 : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
//                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.getContext().report ("user4: not in use");
                }
        };

        const int opcodeUser1 = 0x200016c;
        const int opcodeUser2 = 0x200016d;
        const int opcodeUser3 = 0x200016e;
        const int opcodeUser3Explicit = 0x200016f;
        const int opcodeUser4 = 0x2000170;
        const int opcodeUser4Explicit = 0x2000171;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("user1", "", opcodeUser1);
            extensions.registerInstruction ("user2", "", opcodeUser2);
            extensions.registerInstruction ("user3", "", opcodeUser3, opcodeUser3);
            extensions.registerInstruction ("user4", "", opcodeUser4, opcodeUser4);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeUser1, new OpUser1);
            interpreter.installSegment5 (opcodeUser2, new OpUser2);
            interpreter.installSegment5 (opcodeUser3, new OpUser3<ImplicitRef>);
            interpreter.installSegment5 (opcodeUser3Explicit, new OpUser3<ExplicitRef>);
            interpreter.installSegment5 (opcodeUser4, new OpUser4<ImplicitRef>);
            interpreter.installSegment5 (opcodeUser4Explicit, new OpUser4<ExplicitRef>);
        }
    }
}

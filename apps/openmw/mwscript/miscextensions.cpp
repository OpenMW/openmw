
#include "miscextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "interpretercontext.hpp"

#include "../mwworld/class.hpp"

namespace MWScript
{
    namespace Misc
    {
        class OpXBox : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (0);
                }
        };

        class OpOnActivate : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    runtime.push (context.hasBeenActivated (ptr));
                }
        };

        class OpActivate : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    context.executeActivation();
                }
        };

        class OpLock : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    Interpreter::Type_Integer lockLevel = 100;

                    if (arg0==1)
                    {
                        lockLevel = runtime[0].mInteger;
                        runtime.pop();
                    }

                    MWWorld::Class::get (ptr).lock (ptr, lockLevel);
                }
        };

        class OpLockExplicit : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    Interpreter::Type_Integer lockLevel = 100;

                    if (arg0==1)
                    {
                        lockLevel = runtime[0].mInteger;
                        runtime.pop();
                    }

                    MWWorld::Class::get (ptr).lock (ptr, lockLevel);
                }
        };

        class OpUnlock : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    MWWorld::Class::get (ptr).unlock (ptr);
                }
        };

        class OpUnlockExplicit : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWWorld::Class::get (ptr).unlock (ptr);
                }
        };


        const int opcodeXBox = 0x200000c;
        const int opcodeOnActivate = 0x200000d;
        const int opcodeActivate = 0x2000075;
        const int opcodeLock = 0x20004;
        const int opcodeLockExplicit = 0x20005;
        const int opcodeUnlock = 0x200008c;
        const int opcodeUnlockExplicit = 0x200008d;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("xbox", 'l', "", opcodeXBox);
            extensions.registerFunction ("onactivate", 'l', "", opcodeOnActivate);
            extensions.registerInstruction ("activate", "", opcodeActivate);
            extensions.registerInstruction ("lock", "/l", opcodeLock, opcodeLockExplicit);
            extensions.registerInstruction ("unlock", "", opcodeUnlock, opcodeUnlockExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeXBox, new OpXBox);
            interpreter.installSegment5 (opcodeOnActivate, new OpOnActivate);
            interpreter.installSegment5 (opcodeActivate, new OpActivate);
            interpreter.installSegment3 (opcodeLock, new OpLock);
            interpreter.installSegment3 (opcodeLockExplicit, new OpLockExplicit);
            interpreter.installSegment5 (opcodeUnlock, new OpUnlock);
            interpreter.installSegment5 (opcodeUnlockExplicit, new OpUnlockExplicit);
        }
    }
}

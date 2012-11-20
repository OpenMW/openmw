
#include "miscextensions.hpp"

#include <libs/openengine/ogre/fader.hpp>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Misc
    {
        class OpGetPcSleep : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                runtime.push (MWBase::Environment::get().getWindowManager ()->getPlayerSleeping());
            }
        };

        class OpWakeUpPc : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWBase::Environment::get().getWindowManager ()->wakeUpPlayer();
            }
        };

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

        template<class R>
        class OpLock : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer lockLevel = 100;

                    if (arg0==1)
                    {
                        lockLevel = runtime[0].mInteger;
                        runtime.pop();
                    }

                    MWWorld::Class::get (ptr).lock (ptr, lockLevel);
                }
        };

        template<class R>
        class OpUnlock : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).unlock (ptr);
                }
        };

        class OpToggleCollisionDebug : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_CollisionDebug);

                    context.report (enabled ?
                        "Collision Mesh Rendering -> On" : "Collision Mesh Rendering -> Off");
                }
        };


        class OpToggleCollisionBoxes : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_BoundingBoxes);

                    context.report (enabled ?
                        "Bounding Box Rendering -> On" : "Bounding Box Rendering -> Off");
                }
        };

        class OpToggleWireframe : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_Wireframe);

                    context.report (enabled ?
                        "Wireframe Rendering -> On" : "Wireframe Rendering -> Off");
                }
        };

        class OpTogglePathgrid : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                InterpreterContext& context =
                    static_cast<InterpreterContext&> (runtime.getContext());

                bool enabled =
                    MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_Pathgrid);

                context.report (enabled ?
                    "Path Grid rendering -> On" : "Path Grid Rendering -> Off");
            }
        };

        class OpFadeIn : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->getFader()->fadeIn(time);
                }
        };

        class OpFadeOut : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->getFader()->fadeOut(time);
                }
        };

        class OpFadeTo : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float alpha = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->getFader()->fadeTo(alpha, time);
                }
        };

        class OpToggleWater : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWorld()->toggleWater();
                }
        };

        class OpDontSaveObject : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    // We are ignoring the DontSaveObject statement for now. Probably not worth
                    /// bothering with. The incompatibility we are creating should be marginal at most.
                }
        };

        class OpToggleVanityMode : public Interpreter::Opcode0
        {
            static bool sActivate;

        public:
        
            virtual void execute(Interpreter::Runtime &runtime)
            {
                InterpreterContext& context =
                    static_cast<InterpreterContext&> (runtime.getContext());

                MWBase::World *world =
                    MWBase::Environment::get().getWorld();

                if (world->toggleVanityMode(sActivate, true)) {
                    context.report(
                        (sActivate) ? "Vanity Mode -> On" : "Vanity Mode -> Off"
                    );
                    sActivate = !sActivate;
                } else {
                    context.report("Vanity Mode -> No");
                }
            }
        };
        bool OpToggleVanityMode::sActivate = true;

        const int opcodeXBox = 0x200000c;
        const int opcodeOnActivate = 0x200000d;
        const int opcodeActivate = 0x2000075;
        const int opcodeLock = 0x20004;
        const int opcodeLockExplicit = 0x20005;
        const int opcodeUnlock = 0x200008c;
        const int opcodeUnlockExplicit = 0x200008d;
        const int opcodeToggleCollisionDebug = 0x2000132;
        const int opcodeToggleCollisionBoxes = 0x20001ac;
        const int opcodeToggleWireframe = 0x200013b;
        const int opcodeFadeIn = 0x200013c;
        const int opcodeFadeOut = 0x200013d;
        const int opcodeFadeTo = 0x200013e;
        const int opcodeToggleWater = 0x2000144;
        const int opcodeTogglePathgrid = 0x2000146;
        const int opcodeDontSaveObject = 0x2000153;
        const int opcodeToggleVanityMode = 0x2000174;
        const int opcodeGetPcSleep = 0x200019f;
        const int opcodeWakeUpPc = 0x20001a2;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("xbox", 'l', "", opcodeXBox);
            extensions.registerFunction ("onactivate", 'l', "", opcodeOnActivate);
            extensions.registerInstruction ("activate", "", opcodeActivate);
            extensions.registerInstruction ("lock", "/l", opcodeLock, opcodeLockExplicit);
            extensions.registerInstruction ("unlock", "", opcodeUnlock, opcodeUnlockExplicit);
            extensions.registerInstruction ("togglecollisionboxes", "", opcodeToggleCollisionBoxes);
            extensions.registerInstruction ("togglecollisiongrid", "", opcodeToggleCollisionDebug);
            extensions.registerInstruction ("tcb", "", opcodeToggleCollisionBoxes);
            extensions.registerInstruction ("tcg", "", opcodeToggleCollisionDebug);
            extensions.registerInstruction ("twf", "", opcodeToggleWireframe);
            extensions.registerInstruction ("togglewireframe", "", opcodeToggleWireframe);
            extensions.registerInstruction ("fadein", "f", opcodeFadeIn);
            extensions.registerInstruction ("fadeout", "f", opcodeFadeOut);
            extensions.registerInstruction ("fadeto", "ff", opcodeFadeTo);
            extensions.registerInstruction ("togglewater", "", opcodeToggleWater);
            extensions.registerInstruction ("twa", "", opcodeToggleWater);
            extensions.registerInstruction ("togglepathgrid", "", opcodeTogglePathgrid);
            extensions.registerInstruction ("tpg", "", opcodeTogglePathgrid);
            extensions.registerInstruction ("dontsaveobject", "", opcodeDontSaveObject);
            extensions.registerInstruction ("togglevanitymode", "", opcodeToggleVanityMode);
            extensions.registerInstruction ("tvm", "", opcodeToggleVanityMode);
            extensions.registerFunction ("getpcsleep", 'l', "", opcodeGetPcSleep);
            extensions.registerInstruction ("wakeuppc", "", opcodeWakeUpPc);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeXBox, new OpXBox);
            interpreter.installSegment5 (opcodeOnActivate, new OpOnActivate);
            interpreter.installSegment5 (opcodeActivate, new OpActivate);
            interpreter.installSegment3 (opcodeLock, new OpLock<ImplicitRef>);
            interpreter.installSegment3 (opcodeLockExplicit, new OpLock<ExplicitRef>);
            interpreter.installSegment5 (opcodeUnlock, new OpUnlock<ImplicitRef>);
            interpreter.installSegment5 (opcodeUnlockExplicit, new OpUnlock<ExplicitRef>);
            interpreter.installSegment5 (opcodeToggleCollisionDebug, new OpToggleCollisionDebug);
            interpreter.installSegment5 (opcodeToggleCollisionBoxes, new OpToggleCollisionBoxes);
            interpreter.installSegment5 (opcodeToggleWireframe, new OpToggleWireframe);
            interpreter.installSegment5 (opcodeFadeIn, new OpFadeIn);
            interpreter.installSegment5 (opcodeFadeOut, new OpFadeOut);
            interpreter.installSegment5 (opcodeFadeTo, new OpFadeTo);
            interpreter.installSegment5 (opcodeTogglePathgrid, new OpTogglePathgrid);
            interpreter.installSegment5 (opcodeToggleWater, new OpToggleWater);
            interpreter.installSegment5 (opcodeDontSaveObject, new OpDontSaveObject);
            interpreter.installSegment5 (opcodeToggleVanityMode, new OpToggleVanityMode);
            interpreter.installSegment5 (opcodeGetPcSleep, new OpGetPcSleep);
            interpreter.installSegment5 (opcodeWakeUpPc, new OpWakeUpPc);
        }
    }
}

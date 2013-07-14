
#include "extensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Sound
    {
        template<class R>
        class OpSay : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string file = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string text = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWBase::Environment::get().getSoundManager()->say (ptr, file);

                    if (MWBase::Environment::get().getWindowManager ()->getSubtitlesEnabled())
                        context.messageBox (text);
                }
        };

        template<class R>
        class OpSayDone : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push (MWBase::Environment::get().getSoundManager()->sayDone (ptr));
                }
        };

        class OpStreamMusic : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWBase::Environment::get().getSoundManager()->streamMusic (sound);
                }
        };

        class OpPlaySound : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
                }
        };

        class OpPlaySoundVP : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float volume = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float pitch = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getSoundManager()->playSound (sound, volume, pitch);
                }
        };

        template<class R>
        class OpPlaySound3D : public Interpreter::Opcode0
        {
                bool mLoop;

            public:

                OpPlaySound3D (bool loop) : mLoop (loop) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWBase::Environment::get().getSoundManager()->playSound3D (ptr, sound, 1.0, 1.0, mLoop ? MWBase::SoundManager::Play_Loop :
                                                                                                             MWBase::SoundManager::Play_Normal);
                }
        };

        template<class R>
        class OpPlaySoundVP3D : public Interpreter::Opcode0
        {
                bool mLoop;

            public:

                OpPlaySoundVP3D (bool loop) : mLoop (loop) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float volume = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float pitch = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getSoundManager()->playSound3D (ptr, sound, volume, pitch, mLoop ? MWBase::SoundManager::Play_Loop :
                                                                                                                  MWBase::SoundManager::Play_Normal);

                }
        };

        template<class R>
        class OpStopSound : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWBase::Environment::get().getSoundManager()->stopSound3D (ptr, sound);
                }
        };

        template<class R>
        class OpGetSoundPlaying : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    int index = runtime[0].mInteger;
                    runtime.pop();

                    runtime.push (MWBase::Environment::get().getSoundManager()->getSoundPlaying (
                        ptr, runtime.getStringLiteral (index)));
                }
        };

        const int opcodeSay = 0x2000001;
        const int opcodeSayDone = 0x2000002;
        const int opcodeStreamMusic = 0x2000003;
        const int opcodePlaySound = 0x2000004;
        const int opcodePlaySoundVP = 0x2000005;
        const int opcodePlaySound3D = 0x2000006;
        const int opcodePlaySound3DVP = 0x2000007;
        const int opcodePlayLoopSound3D = 0x2000008;
        const int opcodePlayLoopSound3DVP = 0x2000009;
        const int opcodeStopSound = 0x200000a;
        const int opcodeGetSoundPlaying = 0x200000b;

        const int opcodeSayExplicit = 0x2000019;
        const int opcodeSayDoneExplicit = 0x200001a;
        const int opcodePlaySound3DExplicit = 0x200001b;
        const int opcodePlaySound3DVPExplicit = 0x200001c;
        const int opcodePlayLoopSound3DExplicit = 0x200001d;
        const int opcodePlayLoopSound3DVPExplicit = 0x200001e;
        const int opcodeStopSoundExplicit = 0x200001f;
        const int opcodeGetSoundPlayingExplicit = 0x2000020;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("say", "SS", opcodeSay, opcodeSayExplicit);
            extensions.registerFunction ("saydone", 'l', "", opcodeSayDone, opcodeSayDoneExplicit);
            extensions.registerInstruction ("streammusic", "S", opcodeStreamMusic);
            extensions.registerInstruction ("playsound", "c", opcodePlaySound);
            extensions.registerInstruction ("playsoundvp", "cff", opcodePlaySoundVP);
            extensions.registerInstruction ("playsound3d", "c", opcodePlaySound3D,
                opcodePlaySound3DExplicit);
            extensions.registerInstruction ("playsound3dvp", "cff", opcodePlaySound3DVP,
                opcodePlaySound3DVPExplicit);
            extensions.registerInstruction ("playloopsound3d", "c", opcodePlayLoopSound3D,
                opcodePlayLoopSound3DExplicit);
            extensions.registerInstruction ("playloopsound3dvp", "cff", opcodePlayLoopSound3DVP,
                opcodePlayLoopSound3DVPExplicit);
            extensions.registerInstruction ("stopsound", "c", opcodeStopSound,
                opcodeStopSoundExplicit);
            extensions.registerFunction ("getsoundplaying", 'l', "c", opcodeGetSoundPlaying,
                opcodeGetSoundPlayingExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeSay, new OpSay<ImplicitRef>);
            interpreter.installSegment5 (opcodeSayDone, new OpSayDone<ImplicitRef>);
            interpreter.installSegment5 (opcodeStreamMusic, new OpStreamMusic);
            interpreter.installSegment5 (opcodePlaySound, new OpPlaySound);
            interpreter.installSegment5 (opcodePlaySoundVP, new OpPlaySoundVP);
            interpreter.installSegment5 (opcodePlaySound3D, new OpPlaySound3D<ImplicitRef> (false));
            interpreter.installSegment5 (opcodePlaySound3DVP, new OpPlaySoundVP3D<ImplicitRef> (false));
            interpreter.installSegment5 (opcodePlayLoopSound3D, new OpPlaySound3D<ImplicitRef> (true));
            interpreter.installSegment5 (opcodePlayLoopSound3DVP,
                new OpPlaySoundVP3D<ImplicitRef> (true));
            interpreter.installSegment5 (opcodeStopSound, new OpStopSound<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetSoundPlaying, new OpGetSoundPlaying<ImplicitRef>);

            interpreter.installSegment5 (opcodeSayExplicit, new OpSay<ExplicitRef>);
            interpreter.installSegment5 (opcodeSayDoneExplicit, new OpSayDone<ExplicitRef>);
            interpreter.installSegment5 (opcodePlaySound3DExplicit,
                new OpPlaySound3D<ExplicitRef> (false));
            interpreter.installSegment5 (opcodePlaySound3DVPExplicit,
                new OpPlaySoundVP3D<ExplicitRef> (false));
            interpreter.installSegment5 (opcodePlayLoopSound3DExplicit,
                new OpPlaySound3D<ExplicitRef> (true));
            interpreter.installSegment5 (opcodePlayLoopSound3DVPExplicit,
                new OpPlaySoundVP3D<ExplicitRef> (true));
            interpreter.installSegment5 (opcodeStopSoundExplicit, new OpStopSound<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetSoundPlayingExplicit,
                new OpGetSoundPlaying<ExplicitRef>);
        }
    }
}

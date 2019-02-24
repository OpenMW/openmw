#include "soundextensions.hpp"

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

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

                    MWBase::Environment::get().getSoundManager()->playSound(sound, 1.0, 1.0, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
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

                    MWBase::Environment::get().getSoundManager()->playSound(sound, volume, pitch, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
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

                    MWBase::Environment::get().getSoundManager()->playSound3D(ptr, sound, 1.0, 1.0,
                                                                              MWSound::Type::Sfx,
                                                                              mLoop ? MWSound::PlayMode::LoopRemoveAtDistance
                                                                                    : MWSound::PlayMode::Normal);
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

                    MWBase::Environment::get().getSoundManager()->playSound3D(ptr, sound, volume, pitch,
                                                                              MWSound::Type::Sfx,
                                                                              mLoop ? MWSound::PlayMode::LoopRemoveAtDistance
                                                                                    : MWSound::PlayMode::Normal);

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

                    bool ret = MWBase::Environment::get().getSoundManager()->getSoundPlaying (
                                    ptr, runtime.getStringLiteral (index));

                    // GetSoundPlaying called on an equipped item should also look for sounds played by the equipping actor.
                    if (!ret && ptr.getContainerStore())
                    {
                        MWWorld::Ptr cont = MWBase::Environment::get().getWorld()->findContainer(ptr);

                        if (!cont.isEmpty() && cont.getClass().hasInventoryStore(cont) && cont.getClass().getInventoryStore(cont).isEquipped(ptr))
                        {
                            ret = MWBase::Environment::get().getSoundManager()->getSoundPlaying (
                                        cont, runtime.getStringLiteral (index));
                        }
                    }

                    runtime.push(ret);
                }
        };


        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Sound::opcodeSay, new OpSay<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Sound::opcodeSayDone, new OpSayDone<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Sound::opcodeStreamMusic, new OpStreamMusic);
            interpreter.installSegment5 (Compiler::Sound::opcodePlaySound, new OpPlaySound);
            interpreter.installSegment5 (Compiler::Sound::opcodePlaySoundVP, new OpPlaySoundVP);
            interpreter.installSegment5 (Compiler::Sound::opcodePlaySound3D, new OpPlaySound3D<ImplicitRef> (false));
            interpreter.installSegment5 (Compiler::Sound::opcodePlaySound3DVP, new OpPlaySoundVP3D<ImplicitRef> (false));
            interpreter.installSegment5 (Compiler::Sound::opcodePlayLoopSound3D, new OpPlaySound3D<ImplicitRef> (true));
            interpreter.installSegment5 (Compiler::Sound::opcodePlayLoopSound3DVP,
                new OpPlaySoundVP3D<ImplicitRef> (true));
            interpreter.installSegment5 (Compiler::Sound::opcodeStopSound, new OpStopSound<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Sound::opcodeGetSoundPlaying, new OpGetSoundPlaying<ImplicitRef>);

            interpreter.installSegment5 (Compiler::Sound::opcodeSayExplicit, new OpSay<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Sound::opcodeSayDoneExplicit, new OpSayDone<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Sound::opcodePlaySound3DExplicit,
                new OpPlaySound3D<ExplicitRef> (false));
            interpreter.installSegment5 (Compiler::Sound::opcodePlaySound3DVPExplicit,
                new OpPlaySoundVP3D<ExplicitRef> (false));
            interpreter.installSegment5 (Compiler::Sound::opcodePlayLoopSound3DExplicit,
                new OpPlaySound3D<ExplicitRef> (true));
            interpreter.installSegment5 (Compiler::Sound::opcodePlayLoopSound3DVPExplicit,
                new OpPlaySoundVP3D<ExplicitRef> (true));
            interpreter.installSegment5 (Compiler::Sound::opcodeStopSoundExplicit, new OpStopSound<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Sound::opcodeGetSoundPlayingExplicit,
                new OpGetSoundPlaying<ExplicitRef>);
        }
    }
}

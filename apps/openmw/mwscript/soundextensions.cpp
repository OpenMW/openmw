#include "soundextensions.hpp"

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Sound
    {
        template <class R>
        class OpSay : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWScript::InterpreterContext& context
                    = static_cast<MWScript::InterpreterContext&>(runtime.getContext());

                VFS::Path::Normalized file{ runtime.getStringLiteral(runtime[0].mInteger) };
                runtime.pop();

                std::string_view text = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                MWBase::Environment::get().getSoundManager()->say(ptr, Misc::ResourceHelpers::correctSoundPath(file));

                if (Settings::gui().mSubtitles)
                    context.messageBox(text);
            }
        };

        template <class R>
        class OpSayDone : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                runtime.push(MWBase::Environment::get().getSoundManager()->sayDone(ptr));
            }
        };

        class OpStreamMusic : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const VFS::Path::Normalized music(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWBase::Environment::get().getSoundManager()->streamMusic(
                    Misc::ResourceHelpers::correctMusicPath(music), MWSound::MusicType::MWScript);
            }
        };

        class OpPlaySound : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId sound = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWBase::Environment::get().getSoundManager()->playSound(
                    sound, 1.0, 1.0, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
            }
        };

        class OpPlaySoundVP : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId sound = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Float volume = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float pitch = runtime[0].mFloat;
                runtime.pop();

                MWBase::Environment::get().getSoundManager()->playSound(
                    sound, volume, pitch, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
            }
        };

        template <class R, bool TLoop>
        class OpPlaySound3D : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId sound = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, sound, 1.0, 1.0, MWSound::Type::Sfx,
                    TLoop ? MWSound::PlayMode::LoopRemoveAtDistance : MWSound::PlayMode::Normal);
            }
        };

        template <class R, bool TLoop>
        class OpPlaySoundVP3D : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId sound = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Float volume = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float pitch = runtime[0].mFloat;
                runtime.pop();

                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, sound, volume, pitch, MWSound::Type::Sfx,
                    TLoop ? MWSound::PlayMode::LoopRemoveAtDistance : MWSound::PlayMode::Normal);
            }
        };

        template <class R>
        class OpStopSound : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId sound = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWBase::Environment::get().getSoundManager()->stopSound3D(ptr, sound);
            }
        };

        template <class R>
        class OpGetSoundPlaying : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime, false);

                int index = runtime[0].mInteger;
                runtime.pop();

                if (ptr.isEmpty())
                {
                    runtime.push(0);
                    return;
                }

                bool ret = MWBase::Environment::get().getSoundManager()->getSoundPlaying(
                    ptr, ESM::RefId::stringRefId(runtime.getStringLiteral(index)));

                // GetSoundPlaying called on an equipped item should also look for sounds played by the equipping actor.
                if (!ret && ptr.getContainerStore())
                {
                    MWWorld::Ptr cont = MWBase::Environment::get().getWorld()->findContainer(ptr);

                    if (!cont.isEmpty() && cont.getClass().hasInventoryStore(cont)
                        && cont.getClass().getInventoryStore(cont).isEquipped(ptr))
                    {
                        ret = MWBase::Environment::get().getSoundManager()->getSoundPlaying(
                            cont, ESM::RefId::stringRefId(runtime.getStringLiteral(index)));
                    }
                }

                runtime.push(ret);
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5<OpSay<ImplicitRef>>(Compiler::Sound::opcodeSay);
            interpreter.installSegment5<OpSayDone<ImplicitRef>>(Compiler::Sound::opcodeSayDone);
            interpreter.installSegment5<OpStreamMusic>(Compiler::Sound::opcodeStreamMusic);
            interpreter.installSegment5<OpPlaySound>(Compiler::Sound::opcodePlaySound);
            interpreter.installSegment5<OpPlaySoundVP>(Compiler::Sound::opcodePlaySoundVP);
            interpreter.installSegment5<OpPlaySound3D<ImplicitRef, false>>(Compiler::Sound::opcodePlaySound3D);
            interpreter.installSegment5<OpPlaySoundVP3D<ImplicitRef, false>>(Compiler::Sound::opcodePlaySound3DVP);
            interpreter.installSegment5<OpPlaySound3D<ImplicitRef, true>>(Compiler::Sound::opcodePlayLoopSound3D);
            interpreter.installSegment5<OpPlaySoundVP3D<ImplicitRef, true>>(Compiler::Sound::opcodePlayLoopSound3DVP);
            interpreter.installSegment5<OpStopSound<ImplicitRef>>(Compiler::Sound::opcodeStopSound);
            interpreter.installSegment5<OpGetSoundPlaying<ImplicitRef>>(Compiler::Sound::opcodeGetSoundPlaying);

            interpreter.installSegment5<OpSay<ExplicitRef>>(Compiler::Sound::opcodeSayExplicit);
            interpreter.installSegment5<OpSayDone<ExplicitRef>>(Compiler::Sound::opcodeSayDoneExplicit);
            interpreter.installSegment5<OpPlaySound3D<ExplicitRef, false>>(Compiler::Sound::opcodePlaySound3DExplicit);
            interpreter.installSegment5<OpPlaySoundVP3D<ExplicitRef, false>>(
                Compiler::Sound::opcodePlaySound3DVPExplicit);
            interpreter.installSegment5<OpPlaySound3D<ExplicitRef, true>>(
                Compiler::Sound::opcodePlayLoopSound3DExplicit);
            interpreter.installSegment5<OpPlaySoundVP3D<ExplicitRef, true>>(
                Compiler::Sound::opcodePlayLoopSound3DVPExplicit);
            interpreter.installSegment5<OpStopSound<ExplicitRef>>(Compiler::Sound::opcodeStopSoundExplicit);
            interpreter.installSegment5<OpGetSoundPlaying<ExplicitRef>>(Compiler::Sound::opcodeGetSoundPlayingExplicit);
        }
    }
}

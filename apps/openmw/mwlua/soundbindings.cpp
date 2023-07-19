#include "soundbindings.hpp"
#include "luabindings.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "luamanagerimp.hpp"

namespace MWLua
{
    struct PlaySoundArgs
    {
        bool mScale = true;
        bool mLoop = false;
        float mVolume = 1.f;
        float mPitch = 1.f;
        float mTimeOffset = 0.f;
    };

    PlaySoundArgs getPlaySoundArgs(const sol::optional<sol::table>& options)
    {
        PlaySoundArgs args;

        if (options.has_value())
        {
            args.mLoop = options->get_or("loop", false);
            args.mVolume = options->get_or("volume", 1.f);
            args.mPitch = options->get_or("pitch", 1.f);
            args.mTimeOffset = options->get_or("timeOffset", 0.f);
            args.mScale = options->get_or("scale", true);
        }
        return args;
    }

    MWSound::PlayMode getPlayMode(const PlaySoundArgs& args, bool is3D)
    {
        if (is3D)
        {
            if (args.mLoop)
                return MWSound::PlayMode::LoopRemoveAtDistance;
            return MWSound::PlayMode::Normal;
        }

        if (args.mLoop && !args.mScale)
            return MWSound::PlayMode::LoopNoEnvNoScaling;
        else if (args.mLoop)
            return MWSound::PlayMode::LoopNoEnv;
        else if (!args.mScale)
            return MWSound::PlayMode::NoEnvNoScaling;
        return MWSound::PlayMode::NoEnv;
    }

    sol::table initAmbientPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);

        api["playSound"] = [](std::string_view soundId, const sol::optional<sol::table>& options) {
            auto args = getPlaySoundArgs(options);
            auto playMode = getPlayMode(args, false);
            ESM::RefId sound = ESM::RefId::deserializeText(soundId);

            MWBase::Environment::get().getSoundManager()->playSound(
                sound, args.mVolume, args.mPitch, MWSound::Type::Sfx, playMode, args.mTimeOffset);
        };
        api["playSoundFile"] = [](std::string_view fileName, const sol::optional<sol::table>& options) {
            auto args = getPlaySoundArgs(options);
            auto playMode = getPlayMode(args, false);

            MWBase::Environment::get().getSoundManager()->playSound(
                fileName, args.mVolume, args.mPitch, MWSound::Type::Sfx, playMode, args.mTimeOffset);
        };

        api["stopSound"] = [](std::string_view soundId) {
            ESM::RefId sound = ESM::RefId::deserializeText(soundId);
            MWBase::Environment::get().getSoundManager()->stopSound3D(MWWorld::Ptr(), sound);
        };
        api["stopSoundFile"] = [](std::string_view fileName) {
            MWBase::Environment::get().getSoundManager()->stopSound3D(MWWorld::Ptr(), fileName);
        };

        api["isSoundPlaying"] = [](std::string_view soundId) {
            ESM::RefId sound = ESM::RefId::deserializeText(soundId);
            return MWBase::Environment::get().getSoundManager()->getSoundPlaying(MWWorld::Ptr(), sound);
        };
        api["isSoundFilePlaying"] = [](std::string_view fileName) {
            return MWBase::Environment::get().getSoundManager()->getSoundPlaying(MWWorld::Ptr(), fileName);
        };

        return LuaUtil::makeReadOnly(api);
    }

    sol::table initCoreSoundBindings(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);

        api["playSound3d"]
            = [](std::string_view soundId, const Object& object, const sol::optional<sol::table>& options) {
                  auto args = getPlaySoundArgs(options);
                  auto playMode = getPlayMode(args, true);

                  ESM::RefId sound = ESM::RefId::deserializeText(soundId);

                  MWBase::Environment::get().getSoundManager()->playSound3D(
                      object.ptr(), sound, args.mVolume, args.mPitch, MWSound::Type::Sfx, playMode, args.mTimeOffset);
              };
        api["playSoundFile3d"]
            = [](std::string_view fileName, const Object& object, const sol::optional<sol::table>& options) {
                  auto args = getPlaySoundArgs(options);
                  auto playMode = getPlayMode(args, true);

                  MWBase::Environment::get().getSoundManager()->playSound3D(object.ptr(), fileName, args.mVolume,
                      args.mPitch, MWSound::Type::Sfx, playMode, args.mTimeOffset);
              };

        api["stopSound3d"] = [](std::string_view soundId, const Object& object) {
            ESM::RefId sound = ESM::RefId::deserializeText(soundId);
            MWBase::Environment::get().getSoundManager()->stopSound3D(object.ptr(), sound);
        };
        api["stopSoundFile3d"] = [](std::string_view fileName, const Object& object) {
            MWBase::Environment::get().getSoundManager()->stopSound3D(object.ptr(), fileName);
        };

        api["isSoundPlaying"] = [](std::string_view soundId, const Object& object) {
            ESM::RefId sound = ESM::RefId::deserializeText(soundId);
            return MWBase::Environment::get().getSoundManager()->getSoundPlaying(object.ptr(), sound);
        };
        api["isSoundFilePlaying"] = [](std::string_view fileName, const Object& object) {
            return MWBase::Environment::get().getSoundManager()->getSoundPlaying(object.ptr(), fileName);
        };

        api["say"] = sol::overload(
            [luaManager = context.mLuaManager](
                std::string_view fileName, const Object& object, sol::optional<std::string_view> text) {
                MWBase::Environment::get().getSoundManager()->say(object.ptr(), std::string(fileName));
                if (text)
                    luaManager->addUIMessage(*text);
            },
            [luaManager = context.mLuaManager](std::string_view fileName, sol::optional<std::string_view> text) {
                MWBase::Environment::get().getSoundManager()->say(std::string(fileName));
                if (text)
                    luaManager->addUIMessage(*text);
            });
        api["stopSay"] = sol::overload(
            [](const Object& object) {
                const MWWorld::Ptr& objPtr = object.ptr();
                MWBase::Environment::get().getSoundManager()->stopSay(objPtr);
            },
            []() { MWBase::Environment::get().getSoundManager()->stopSay(MWWorld::ConstPtr()); });
        api["isSayActive"] = sol::overload(
            [](const Object& object) {
                const MWWorld::Ptr& objPtr = object.ptr();
                return MWBase::Environment::get().getSoundManager()->sayActive(objPtr);
            },
            []() { return MWBase::Environment::get().getSoundManager()->sayActive(MWWorld::ConstPtr()); });

        return LuaUtil::makeReadOnly(api);
    }
}

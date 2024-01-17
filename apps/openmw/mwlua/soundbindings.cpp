#include "soundbindings.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/esm3/loadsoun.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/pathutil.hpp>

#include "luamanagerimp.hpp"

namespace
{
    struct PlaySoundArgs
    {
        bool mScale = true;
        bool mLoop = false;
        float mVolume = 1.f;
        float mPitch = 1.f;
        float mTimeOffset = 0.f;
    };

    struct StreamMusicArgs
    {
        float mFade = 1.f;
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

    StreamMusicArgs getStreamMusicArgs(const sol::optional<sol::table>& options)
    {
        StreamMusicArgs args;

        if (options.has_value())
        {
            args.mFade = options->get_or("fadeOut", 1.f);
        }
        return args;
    }
}

namespace MWLua
{
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

        api["streamMusic"] = [](std::string_view fileName, const sol::optional<sol::table>& options) {
            auto args = getStreamMusicArgs(options);
            MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
            sndMgr->streamMusic(std::string(fileName), MWSound::MusicType::Scripted, args.mFade);
        };

        api["isMusicPlaying"] = []() { return MWBase::Environment::get().getSoundManager()->isMusicPlaying(); };

        api["stopMusic"] = []() { MWBase::Environment::get().getSoundManager()->stopMusic(); };

        return LuaUtil::makeReadOnly(api);
    }

    sol::table initCoreSoundBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table api(lua, sol::create);

        api["isEnabled"] = []() { return MWBase::Environment::get().getSoundManager()->isEnabled(); };

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

        using SoundStore = MWWorld::Store<ESM::Sound>;
        sol::usertype<SoundStore> soundStoreT = lua.new_usertype<SoundStore>("ESM3_SoundStore");
        soundStoreT[sol::meta_function::to_string]
            = [](const SoundStore& store) { return "ESM3_SoundStore{" + std::to_string(store.getSize()) + " sounds}"; };
        soundStoreT[sol::meta_function::length] = [](const SoundStore& store) { return store.getSize(); };
        soundStoreT[sol::meta_function::index] = sol::overload(
            [](const SoundStore& store, size_t index) -> const ESM::Sound* {
                if (index == 0 || index > store.getSize())
                    return nullptr;
                return store.at(index - 1);
            },
            [](const SoundStore& store, std::string_view soundId) -> const ESM::Sound* {
                return store.search(ESM::RefId::deserializeText(soundId));
            });
        soundStoreT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        soundStoreT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();

        api["sounds"] = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Sound>();

        // Sound record
        auto soundT = lua.new_usertype<ESM::Sound>("ESM3_Sound");
        soundT[sol::meta_function::to_string]
            = [](const ESM::Sound& rec) -> std::string { return "ESM3_Sound[" + rec.mId.toDebugString() + "]"; };
        soundT["id"] = sol::readonly_property([](const ESM::Sound& rec) { return rec.mId.serializeText(); });
        soundT["volume"]
            = sol::readonly_property([](const ESM::Sound& rec) -> unsigned char { return rec.mData.mVolume; });
        soundT["minRange"]
            = sol::readonly_property([](const ESM::Sound& rec) -> unsigned char { return rec.mData.mMinRange; });
        soundT["maxRange"]
            = sol::readonly_property([](const ESM::Sound& rec) -> unsigned char { return rec.mData.mMaxRange; });
        soundT["fileName"] = sol::readonly_property([](const ESM::Sound& rec) -> std::string {
            return VFS::Path::normalizeFilename(Misc::ResourceHelpers::correctSoundPath(rec.mSound));
        });

        return LuaUtil::makeReadOnly(api);
    }
}

#include "soundbindings.hpp"
#include "recordstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/esm3/loadsoun.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/pathutil.hpp>

#include "luamanagerimp.hpp"
#include "objectvariant.hpp"

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

    MWWorld::Ptr getMutablePtrOrThrow(const MWLua::ObjectVariant& variant)
    {
        if (variant.isLObject())
            throw std::runtime_error("Local scripts can only modify object they are attached to.");

        MWWorld::Ptr ptr = variant.ptr();
        if (ptr.isEmpty())
            throw std::runtime_error("Invalid object");

        return ptr;
    }

    MWWorld::Ptr getPtrOrThrow(const MWLua::ObjectVariant& variant)
    {
        MWWorld::Ptr ptr = variant.ptr();
        if (ptr.isEmpty())
            throw std::runtime_error("Invalid object");

        return ptr;
    }

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
        sol::state_view& lua = context.mLua->sol();
        if (lua["openmw_ambient"] != sol::nil)
            return lua["openmw_ambient"];

        sol::table api(lua, sol::create);

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
            sndMgr->streamMusic(VFS::Path::Normalized(fileName), MWSound::MusicType::Normal, args.mFade);
        };

        api["say"]
            = [luaManager = context.mLuaManager](std::string_view fileName, sol::optional<std::string_view> text) {
                  MWBase::Environment::get().getSoundManager()->say(VFS::Path::Normalized(fileName));
                  if (text)
                      luaManager->addUIMessage(*text);
              };

        api["stopSay"] = []() { MWBase::Environment::get().getSoundManager()->stopSay(MWWorld::ConstPtr()); };
        api["isSayActive"]
            = []() { return MWBase::Environment::get().getSoundManager()->sayActive(MWWorld::ConstPtr()); };

        api["isMusicPlaying"] = []() { return MWBase::Environment::get().getSoundManager()->isMusicPlaying(); };

        api["stopMusic"] = []() { MWBase::Environment::get().getSoundManager()->stopMusic(); };

        lua["openmw_ambient"] = LuaUtil::makeReadOnly(api);
        return lua["openmw_ambient"];
    }

    sol::table initCoreSoundBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table api(lua, sol::create);

        api["isEnabled"] = []() { return MWBase::Environment::get().getSoundManager()->isEnabled(); };

        api["playSound3d"]
            = [](std::string_view soundId, const sol::object& object, const sol::optional<sol::table>& options) {
                  auto args = getPlaySoundArgs(options);
                  auto playMode = getPlayMode(args, true);

                  ESM::RefId sound = ESM::RefId::deserializeText(soundId);
                  MWWorld::Ptr ptr = getMutablePtrOrThrow(ObjectVariant(object));

                  MWBase::Environment::get().getSoundManager()->playSound3D(
                      ptr, sound, args.mVolume, args.mPitch, MWSound::Type::Sfx, playMode, args.mTimeOffset);
              };
        api["playSoundFile3d"]
            = [](std::string_view fileName, const sol::object& object, const sol::optional<sol::table>& options) {
                  auto args = getPlaySoundArgs(options);
                  auto playMode = getPlayMode(args, true);
                  MWWorld::Ptr ptr = getMutablePtrOrThrow(ObjectVariant(object));

                  MWBase::Environment::get().getSoundManager()->playSound3D(
                      ptr, fileName, args.mVolume, args.mPitch, MWSound::Type::Sfx, playMode, args.mTimeOffset);
              };

        api["stopSound3d"] = [](std::string_view soundId, const sol::object& object) {
            ESM::RefId sound = ESM::RefId::deserializeText(soundId);
            MWWorld::Ptr ptr = getMutablePtrOrThrow(ObjectVariant(object));
            MWBase::Environment::get().getSoundManager()->stopSound3D(ptr, sound);
        };
        api["stopSoundFile3d"] = [](std::string_view fileName, const sol::object& object) {
            MWWorld::Ptr ptr = getMutablePtrOrThrow(ObjectVariant(object));
            MWBase::Environment::get().getSoundManager()->stopSound3D(ptr, fileName);
        };

        api["isSoundPlaying"] = [](std::string_view soundId, const sol::object& object) {
            ESM::RefId sound = ESM::RefId::deserializeText(soundId);
            const MWWorld::Ptr& ptr = getPtrOrThrow(ObjectVariant(object));
            return MWBase::Environment::get().getSoundManager()->getSoundPlaying(ptr, sound);
        };
        api["isSoundFilePlaying"] = [](std::string_view fileName, const sol::object& object) {
            const MWWorld::Ptr& ptr = getPtrOrThrow(ObjectVariant(object));
            return MWBase::Environment::get().getSoundManager()->getSoundPlaying(ptr, fileName);
        };

        api["say"] = [luaManager = context.mLuaManager](
                         std::string_view fileName, const sol::object& object, sol::optional<std::string_view> text) {
            MWWorld::Ptr ptr = getMutablePtrOrThrow(ObjectVariant(object));
            MWBase::Environment::get().getSoundManager()->say(ptr, VFS::Path::Normalized(fileName));
            if (text)
                luaManager->addUIMessage(*text);
        };
        api["stopSay"] = [](const sol::object& object) {
            MWWorld::Ptr ptr = getMutablePtrOrThrow(ObjectVariant(object));
            MWBase::Environment::get().getSoundManager()->stopSay(ptr);
        };
        api["isSayActive"] = [](const sol::object& object) {
            const MWWorld::Ptr& ptr = getPtrOrThrow(ObjectVariant(object));
            return MWBase::Environment::get().getSoundManager()->sayActive(ptr);
        };

        addRecordFunctionBinding<ESM::Sound>(api, context);

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
            return Misc::ResourceHelpers::correctSoundPath(VFS::Path::Normalized(rec.mSound)).value();
        });

        return LuaUtil::makeReadOnly(api);
    }
}

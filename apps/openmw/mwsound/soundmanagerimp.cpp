#include "soundmanagerimp.hpp"

#include <algorithm>
#include <map>
#include <numeric>
#include <sstream>

#include <osg/Matrixf>

#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/debug/debuglog.hpp>
#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "sound_buffer.hpp"
#include "sound_decoder.hpp"
#include "sound_output.hpp"
#include "sound.hpp"

#include "openal_output.hpp"
#include "ffmpeg_decoder.hpp"


namespace MWSound
{
    namespace
    {
        constexpr float sMinUpdateInterval = 1.0f / 30.0f;
        constexpr float sSfxFadeInDuration = 1.0f;
        constexpr float sSfxFadeOutDuration = 1.0f;

        WaterSoundUpdaterSettings makeWaterSoundUpdaterSettings()
        {
            WaterSoundUpdaterSettings settings;

            settings.mNearWaterRadius = Fallback::Map::getInt("Water_NearWaterRadius");
            settings.mNearWaterPoints = Fallback::Map::getInt("Water_NearWaterPoints");
            settings.mNearWaterIndoorTolerance = Fallback::Map::getFloat("Water_NearWaterIndoorTolerance");
            settings.mNearWaterOutdoorTolerance = Fallback::Map::getFloat("Water_NearWaterOutdoorTolerance");
            settings.mNearWaterIndoorID = Misc::StringUtils::lowerCase(Fallback::Map::getString("Water_NearWaterIndoorID"));
            settings.mNearWaterOutdoorID = Misc::StringUtils::lowerCase(Fallback::Map::getString("Water_NearWaterOutdoorID"));

            return settings;
        }

        float initialFadeVolume(float squaredDist, Sound_Buffer *sfx, Type type, PlayMode mode)
        {
            // If a sound is farther away than its maximum distance, start playing it with a zero fade volume.
            // It can still become audible once the player moves closer.
            const float maxDist = sfx->getMaxDist();
            if (squaredDist > (maxDist * maxDist))
                return 0.0f;

            // This is a *heuristic* that causes environment sounds to fade in. The idea is the following:
            // - Only looped sounds playing through the effects channel are environment sounds
            // - Do not fade in sounds if the player is already so close that the sound plays at maximum volume
            const float minDist = sfx->getMinDist();
            if ((squaredDist > (minDist * minDist)) && (type == Type::Sfx) && (mode & PlayMode::Loop))
                return 0.0f;

            return 1.0;
        }
    }

    // For combining PlayMode and Type flags
    inline int operator|(PlayMode a, Type b) { return static_cast<int>(a) | static_cast<int>(b); }

    SoundManager::SoundManager(const VFS::Manager* vfs, bool useSound)
        : mVFS(vfs)
        , mOutput(new OpenAL_Output(*this))
        , mWaterSoundUpdater(makeWaterSoundUpdaterSettings())
        , mSoundBuffers(*vfs, *mOutput)
        , mListenerUnderwater(false)
        , mListenerPos(0,0,0)
        , mListenerDir(1,0,0)
        , mListenerUp(0,0,1)
        , mUnderwaterSound(nullptr)
        , mNearWaterSound(nullptr)
        , mPlaybackPaused(false)
        , mTimePassed(0.f)
        , mLastCell(nullptr)
        , mCurrentRegionSound(nullptr)
    {
        if(!useSound)
        {
            Log(Debug::Info) << "Sound disabled.";
            return;
        }

        std::string hrtfname = Settings::Manager::getString("hrtf", "Sound");
        int hrtfstate = Settings::Manager::getInt("hrtf enable", "Sound");
        HrtfMode hrtfmode = hrtfstate < 0 ? HrtfMode::Auto :
                            hrtfstate > 0 ? HrtfMode::Enable : HrtfMode::Disable;

        std::string devname = Settings::Manager::getString("device", "Sound");
        if(!mOutput->init(devname, hrtfname, hrtfmode))
        {
            Log(Debug::Error) << "Failed to initialize audio output, sound disabled";
            return;
        }

        std::vector<std::string> names = mOutput->enumerate();
        std::stringstream stream;

        stream << "Enumerated output devices:\n";
        for(const std::string &name : names)
            stream << "  " << name;

        Log(Debug::Info) << stream.str();
        stream.str("");

        names = mOutput->enumerateHrtf();
        if(!names.empty())
        {
            stream << "Enumerated HRTF names:\n";
            for(const std::string &name : names)
                stream << "  " << name;

            Log(Debug::Info) << stream.str();
        }
    }

    SoundManager::~SoundManager()
    {
        SoundManager::clear();
        mSoundBuffers.clear();
        mOutput.reset();
    }

    // Return a new decoder instance, used as needed by the output implementations
    DecoderPtr SoundManager::getDecoder()
    {
        return std::make_shared<FFmpeg_Decoder>(mVFS);
    }

    DecoderPtr SoundManager::loadVoice(const std::string &voicefile)
    {
        try
        {
            DecoderPtr decoder = getDecoder();
            decoder->open(Misc::ResourceHelpers::correctSoundPath(voicefile, decoder->mResourceMgr));
            return decoder;
        }
        catch(std::exception &e)
        {
            Log(Debug::Error) << "Failed to load audio from " << voicefile << ": " << e.what();
        }

        return nullptr;
    }

    SoundPtr SoundManager::getSoundRef()
    {
        return mSounds.get();
    }

    StreamPtr SoundManager::getStreamRef()
    {
        return mStreams.get();
    }

    StreamPtr SoundManager::playVoice(DecoderPtr decoder, const osg::Vec3f &pos, bool playlocal)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        static const float fAudioMinDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMinDistanceMult")->mValue.getFloat();
        static const float fAudioMaxDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMaxDistanceMult")->mValue.getFloat();
        static const float fAudioVoiceDefaultMinDistance = world->getStore().get<ESM::GameSetting>().find("fAudioVoiceDefaultMinDistance")->mValue.getFloat();
        static const float fAudioVoiceDefaultMaxDistance = world->getStore().get<ESM::GameSetting>().find("fAudioVoiceDefaultMaxDistance")->mValue.getFloat();
        static float minDistance = std::max(fAudioVoiceDefaultMinDistance * fAudioMinDistanceMult, 1.0f);
        static float maxDistance = std::max(fAudioVoiceDefaultMaxDistance * fAudioMaxDistanceMult, minDistance);

        bool played;
        float basevol = volumeFromType(Type::Voice);
        StreamPtr sound = getStreamRef();
        if(playlocal)
        {
            sound->init([&] {
                SoundParams params;
                params.mBaseVolume = basevol;
                params.mFlags = PlayMode::NoEnv | Type::Voice | Play_2D;
                return params;
            } ());
            played = mOutput->streamSound(decoder, sound.get(), true);
        }
        else
        {
            sound->init([&] {
                SoundParams params;
                params.mPos = pos;
                params.mBaseVolume = basevol;
                params.mMinDistance = minDistance;
                params.mMaxDistance = maxDistance;
                params.mFlags = PlayMode::Normal | Type::Voice | Play_3D;
                return params;
            } ());
            played = mOutput->streamSound3D(decoder, sound.get(), true);
        }
        if(!played)
            return nullptr;
        return sound;
    }

    // Gets the combined volume settings for the given sound type
    float SoundManager::volumeFromType(Type type) const
    {
        return mVolumeSettings.getVolumeFromType(type);
    }

    void SoundManager::stopMusic()
    {
        if(mMusic)
        {
            mOutput->finishStream(mMusic.get());
            mMusic = nullptr;
        }
    }

    void SoundManager::streamMusicFull(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        Log(Debug::Info) << "Playing " << filename;
        mLastPlayedMusic = filename;

        stopMusic();

        DecoderPtr decoder = getDecoder();
        try
        {
            decoder->open(filename);
        }
        catch(std::exception &e)
        {
            Log(Debug::Error) << "Failed to load audio from " << filename << ": " << e.what();
            return;
        }


        mMusic = getStreamRef();
        mMusic->init([&] {
            SoundParams params;
            params.mBaseVolume = volumeFromType(Type::Music);
            params.mFlags = PlayMode::NoEnvNoScaling | Type::Music | Play_2D;
            return params;
        } ());
        mOutput->streamSound(decoder, mMusic.get());
    }

    void SoundManager::advanceMusic(const std::string& filename)
    {
        if (!isMusicPlaying())
        {
            streamMusicFull(filename);
            return;
        }

        mNextMusic = filename;

        mMusic->setFadeout(1.f);
    }

    void SoundManager::startRandomTitle()
    {
        const std::vector<std::string> &filelist = mMusicFiles[mCurrentPlaylist];
        auto &tracklist = mMusicToPlay[mCurrentPlaylist];

        // Do a Fisher-Yates shuffle

        // Repopulate if playlist is empty
        if(tracklist.empty())
        {
            tracklist.resize(filelist.size());
            std::iota(tracklist.begin(), tracklist.end(), 0);
        }

        int i = Misc::Rng::rollDice(tracklist.size());

        // Reshuffle if last played music is the same after a repopulation
        if(filelist[tracklist[i]] == mLastPlayedMusic)
            i = (i+1) % tracklist.size();

        // Remove music from list after advancing music
        advanceMusic(filelist[tracklist[i]]);
        tracklist[i] = tracklist.back();
        tracklist.pop_back();
    }


    void SoundManager::streamMusic(const std::string& filename)
    {
        advanceMusic("Music/"+filename);
    }

    bool SoundManager::isMusicPlaying()
    {
        return mMusic && mOutput->isStreamPlaying(mMusic.get());
    }

    void SoundManager::playPlaylist(const std::string &playlist)
    {
        if (mCurrentPlaylist == playlist)
            return;

        if (mMusicFiles.find(playlist) == mMusicFiles.end())
        {
            std::vector<std::string> filelist;

            for (const auto& name : mVFS->getRecursiveDirectoryIterator("Music/" + playlist))
                filelist.push_back(name);

            mMusicFiles[playlist] = filelist;
        }

        if (mMusicFiles[playlist].empty())
            return;

        mCurrentPlaylist = playlist;
        startRandomTitle();
    }

    void SoundManager::playTitleMusic()
    {
        if (mCurrentPlaylist == "Title")
            return;

        if (mMusicFiles.find("Title") == mMusicFiles.end())
        {
            std::vector<std::string> filelist;
            // Is there an ini setting for this filename or something?
            std::string filename = "music/special/morrowind title.mp3";
            if (mVFS->exists(filename))
            {
                filelist.emplace_back(filename);
                mMusicFiles["Title"] = filelist;
            }
            else
            {
                Log(Debug::Warning) << "Title music not found";
                return;
            }
        }

        if (mMusicFiles["Title"].empty())
            return;

        mCurrentPlaylist = "Title";
        startRandomTitle();
    }

    void SoundManager::say(const MWWorld::ConstPtr &ptr, const std::string &filename)
    {
        if(!mOutput->isInitialized())
            return;

        DecoderPtr decoder = loadVoice(mVFS->normalizeFilename("Sound/" + filename));
        if (!decoder)
            return;

        MWBase::World *world = MWBase::Environment::get().getWorld();
        const osg::Vec3f pos = world->getActorHeadTransform(ptr).getTrans();

        stopSay(ptr);
        StreamPtr sound = playVoice(decoder, pos, (ptr == MWMechanics::getPlayer()));
        if(!sound) return;

        mSaySoundsQueue.emplace(ptr.mRef, SaySound {ptr.mCell, std::move(sound)});
    }

    float SoundManager::getSaySoundLoudness(const MWWorld::ConstPtr &ptr) const
    {
        SaySoundMap::const_iterator snditer = mActiveSaySounds.find(ptr.mRef);
        if(snditer != mActiveSaySounds.end())
        {
            Stream *sound = snditer->second.mStream.get();
            return mOutput->getStreamLoudness(sound);
        }

        return 0.0f;
    }

    void SoundManager::say(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;

        DecoderPtr decoder = loadVoice(mVFS->normalizeFilename("Sound/" + filename));
        if (!decoder)
            return;

        stopSay(MWWorld::ConstPtr());
        StreamPtr sound = playVoice(decoder, osg::Vec3f(), true);
        if(!sound) return;

        mActiveSaySounds.emplace(nullptr, SaySound {nullptr, std::move(sound)});
    }

    bool SoundManager::sayDone(const MWWorld::ConstPtr &ptr) const
    {
        SaySoundMap::const_iterator snditer = mActiveSaySounds.find(ptr.mRef);
        if(snditer != mActiveSaySounds.end())
        {
            if(mOutput->isStreamPlaying(snditer->second.mStream.get()))
                return false;
            return true;
        }
        return true;
    }

    bool SoundManager::sayActive(const MWWorld::ConstPtr &ptr) const
    {
        SaySoundMap::const_iterator snditer = mSaySoundsQueue.find(ptr.mRef);
        if(snditer != mSaySoundsQueue.end())
        {
            if(mOutput->isStreamPlaying(snditer->second.mStream.get()))
                return true;
            return false;
        }

        snditer = mActiveSaySounds.find(ptr.mRef);
        if(snditer != mActiveSaySounds.end())
        {
            if(mOutput->isStreamPlaying(snditer->second.mStream.get()))
                return true;
            return false;
        }

        return false;
    }

    void SoundManager::stopSay(const MWWorld::ConstPtr &ptr)
    {
        SaySoundMap::iterator snditer = mSaySoundsQueue.find(ptr.mRef);
        if(snditer != mSaySoundsQueue.end())
        {
            mOutput->finishStream(snditer->second.mStream.get());
            mSaySoundsQueue.erase(snditer);
        }

        snditer = mActiveSaySounds.find(ptr.mRef);
        if(snditer != mActiveSaySounds.end())
        {
            mOutput->finishStream(snditer->second.mStream.get());
            mActiveSaySounds.erase(snditer);
        }
    }


    Stream *SoundManager::playTrack(const DecoderPtr& decoder, Type type)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        StreamPtr track = getStreamRef();
        track->init([&] {
            SoundParams params;
            params.mBaseVolume = volumeFromType(type);
            params.mFlags = PlayMode::NoEnvNoScaling | type | Play_2D;
            return params;
        } ());
        if(!mOutput->streamSound(decoder, track.get()))
            return nullptr;

        Stream* result = track.get();
        const auto it = std::lower_bound(mActiveTracks.begin(), mActiveTracks.end(), track);
        mActiveTracks.insert(it, std::move(track));
        return result;
    }

    void SoundManager::stopTrack(Stream *stream)
    {
        mOutput->finishStream(stream);
        TrackList::iterator iter = std::lower_bound(mActiveTracks.begin(), mActiveTracks.end(), stream,
                                                    [] (const StreamPtr& lhs, Stream* rhs) { return lhs.get() < rhs; });
        if(iter != mActiveTracks.end() && iter->get() == stream)
            mActiveTracks.erase(iter);
    }

    double SoundManager::getTrackTimeDelay(Stream *stream)
    {
        return mOutput->getStreamDelay(stream);
    }


    Sound* SoundManager::playSound(std::string_view soundId, float volume, float pitch, Type type, PlayMode mode, float offset)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        Sound_Buffer *sfx = mSoundBuffers.load(Misc::StringUtils::lowerCase(soundId));
        if(!sfx) return nullptr;

        // Only one copy of given sound can be played at time, so stop previous copy
        stopSound(sfx, MWWorld::ConstPtr());

        SoundPtr sound = getSoundRef();
        sound->init([&] {
            SoundParams params;
            params.mVolume = volume * sfx->getVolume();
            params.mBaseVolume = volumeFromType(type);
            params.mPitch = pitch;
            params.mFlags = mode | type | Play_2D;
            return params;
        } ());
        if(!mOutput->playSound(sound.get(), sfx->getHandle(), offset))
            return nullptr;

        Sound* result = sound.get();
        mActiveSounds[nullptr].mList.emplace_back(std::move(sound), sfx);
        mSoundBuffers.use(*sfx);
        return result;
    }

    Sound *SoundManager::playSound3D(const MWWorld::ConstPtr &ptr, std::string_view soundId,
                                     float volume, float pitch, Type type, PlayMode mode,
                                     float offset)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        const osg::Vec3f objpos(ptr.getRefData().getPosition().asVec3());
        const float squaredDist = (mListenerPos - objpos).length2();
        if ((mode & PlayMode::RemoveAtDistance) && squaredDist > 2000 * 2000)
            return nullptr;

        // Look up the sound in the ESM data
        Sound_Buffer *sfx = mSoundBuffers.load(Misc::StringUtils::lowerCase(soundId));
        if(!sfx) return nullptr;

        // Only one copy of given sound can be played at time on ptr, so stop previous copy
        stopSound(sfx, ptr);

        bool played;
        SoundPtr sound = getSoundRef();
        if(!(mode&PlayMode::NoPlayerLocal) && ptr == MWMechanics::getPlayer())
        {
            sound->init([&] {
                SoundParams params;
                params.mVolume = volume * sfx->getVolume();
                params.mBaseVolume = volumeFromType(type);
                params.mPitch = pitch;
                params.mFlags = mode | type | Play_2D;
                return params;
            } ());
            played = mOutput->playSound(sound.get(), sfx->getHandle(), offset);
        }
        else
        {
            sound->init([&] {
                SoundParams params;
                params.mPos = objpos;
                params.mVolume = volume * sfx->getVolume();
                params.mBaseVolume = volumeFromType(type);
                params.mFadeVolume = initialFadeVolume(squaredDist, sfx, type, mode);
                params.mPitch = pitch;
                params.mMinDistance = sfx->getMinDist();
                params.mMaxDistance = sfx->getMaxDist();
                params.mFlags = mode | type | Play_3D;
                return params;
            } ());
            played = mOutput->playSound3D(sound.get(), sfx->getHandle(), offset);
        }
        if(!played)
            return nullptr;

        Sound* result = sound.get();
        auto it = mActiveSounds.find(ptr.mRef);
        if (it == mActiveSounds.end())
            it = mActiveSounds.emplace(ptr.mRef, ActiveSound {ptr.mCell, {}}).first;
        it->second.mList.emplace_back(std::move(sound), sfx);
        mSoundBuffers.use(*sfx);
        return result;
    }

    Sound *SoundManager::playSound3D(const osg::Vec3f& initialPos, std::string_view soundId,
                                     float volume, float pitch, Type type, PlayMode mode,
                                     float offset)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        // Look up the sound in the ESM data
        Sound_Buffer *sfx = mSoundBuffers.load(Misc::StringUtils::lowerCase(soundId));
        if(!sfx) return nullptr;

        const float squaredDist = (mListenerPos - initialPos).length2();

        SoundPtr sound = getSoundRef();
        sound->init([&] {
            SoundParams params;
            params.mPos = initialPos;
            params.mVolume = volume * sfx->getVolume();
            params.mBaseVolume = volumeFromType(type);
            params.mFadeVolume = initialFadeVolume(squaredDist, sfx, type, mode);
            params.mPitch = pitch;
            params.mMinDistance = sfx->getMinDist();
            params.mMaxDistance = sfx->getMaxDist();
            params.mFlags = mode | type | Play_3D;
            return params;
        } ());
        if(!mOutput->playSound3D(sound.get(), sfx->getHandle(), offset))
            return nullptr;

        Sound* result = sound.get();
        mActiveSounds[nullptr].mList.emplace_back(std::move(sound), sfx);
        mSoundBuffers.use(*sfx);
        return result;
    }

    void SoundManager::stopSound(Sound *sound)
    {
        if(sound)
            mOutput->finishSound(sound);
    }

    void SoundManager::stopSound(Sound_Buffer *sfx, const MWWorld::ConstPtr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.find(ptr.mRef);
        if(snditer != mActiveSounds.end())
        {
            for (SoundBufferRefPair &snd : snditer->second.mList)
            {
                if(snd.second == sfx)
                    mOutput->finishSound(snd.first.get());
            }
        }
    }

    void SoundManager::stopSound3D(const MWWorld::ConstPtr &ptr, std::string_view soundId)
    {
        if(!mOutput->isInitialized())
            return;

        Sound_Buffer *sfx = mSoundBuffers.lookup(Misc::StringUtils::lowerCase(soundId));
        if (!sfx) return;

        stopSound(sfx, ptr);
    }

    void SoundManager::stopSound3D(const MWWorld::ConstPtr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.find(ptr.mRef);
        if(snditer != mActiveSounds.end())
        {
            for (SoundBufferRefPair &snd : snditer->second.mList)
                mOutput->finishSound(snd.first.get());
        }
        SaySoundMap::iterator sayiter = mSaySoundsQueue.find(ptr.mRef);
        if(sayiter != mSaySoundsQueue.end())
            mOutput->finishStream(sayiter->second.mStream.get());
        sayiter = mActiveSaySounds.find(ptr.mRef);
        if(sayiter != mActiveSaySounds.end())
            mOutput->finishStream(sayiter->second.mStream.get());
    }

    void SoundManager::stopSound(const MWWorld::CellStore *cell)
    {
        for (auto& [ref, sound] : mActiveSounds)
        {
            if (ref != nullptr && ref != MWMechanics::getPlayer().mRef && sound.mCell == cell)
            {
                for (SoundBufferRefPair& sndbuf : sound.mList)
                    mOutput->finishSound(sndbuf.first.get());
            }
        }

        for (const auto& [ref, sound] : mSaySoundsQueue)
        {
            if (ref != nullptr && ref != MWMechanics::getPlayer().mRef && sound.mCell == cell)
                mOutput->finishStream(sound.mStream.get());
        }

        for (const auto& [ref, sound]: mActiveSaySounds)
        {
            if (ref != nullptr && ref != MWMechanics::getPlayer().mRef && sound.mCell == cell)
                mOutput->finishStream(sound.mStream.get());
        }
    }

    void SoundManager::fadeOutSound3D(const MWWorld::ConstPtr &ptr,
            std::string_view soundId, float duration)
    {
        SoundMap::iterator snditer = mActiveSounds.find(ptr.mRef);
        if(snditer != mActiveSounds.end())
        {
            Sound_Buffer *sfx = mSoundBuffers.lookup(Misc::StringUtils::lowerCase(soundId));
            if (sfx == nullptr)
                return;
            for (SoundBufferRefPair &sndbuf : snditer->second.mList)
            {
                if(sndbuf.second == sfx)
                    sndbuf.first->setFadeout(duration);
            }
        }
    }

    bool SoundManager::getSoundPlaying(const MWWorld::ConstPtr &ptr, std::string_view soundId) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.find(ptr.mRef);
        if(snditer != mActiveSounds.end())
        {
            Sound_Buffer *sfx = mSoundBuffers.lookup(Misc::StringUtils::lowerCase(soundId));
            return std::find_if(snditer->second.mList.cbegin(), snditer->second.mList.cend(),
                [this,sfx](const SoundBufferRefPair &snd) -> bool
                { return snd.second == sfx && mOutput->isSoundPlaying(snd.first.get()); }
            ) != snditer->second.mList.cend();
        }
        return false;
    }

    void SoundManager::pauseSounds(BlockerType blocker, int types)
    {
        if(mOutput->isInitialized())
        {
            if (mPausedSoundTypes[blocker] != 0)
                resumeSounds(blocker);

            types = types & Type::Mask;
            mOutput->pauseSounds(types);
            mPausedSoundTypes[blocker] = types;
        }
    }

    void SoundManager::resumeSounds(BlockerType blocker)
    {
        if(mOutput->isInitialized())
        {
            mPausedSoundTypes[blocker] = 0;
            int types = int(Type::Mask);
            for (int currentBlocker = 0; currentBlocker < BlockerType::MaxCount; currentBlocker++)
            {
                if (currentBlocker != blocker)
                    types &= ~mPausedSoundTypes[currentBlocker];
            }

            mOutput->resumeSounds(types);
        }
    }

    void SoundManager::pausePlayback()
    {
        if (mPlaybackPaused)
            return;

        mPlaybackPaused = true;
        mOutput->pauseActiveDevice();
    }

    void SoundManager::resumePlayback()
    {
        if (!mPlaybackPaused)
            return;

        mPlaybackPaused = false;
        mOutput->resumeActiveDevice();
    }

    void SoundManager::updateRegionSound(float duration)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::ConstPtr player = world->getPlayerPtr();
        const ESM::Cell *cell = player.getCell()->getCell();

        if (!cell->isExterior())
            return;
        if (mCurrentRegionSound && mOutput->isSoundPlaying(mCurrentRegionSound))
            return;

        if (const auto next = mRegionSoundSelector.getNextRandom(duration, cell->mRegion, *world))
            mCurrentRegionSound = playSound(*next, 1.0f, 1.0f);
    }

    void SoundManager::updateWaterSound()
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::ConstPtr player = world->getPlayerPtr();
        const ESM::Cell *curcell = player.getCell()->getCell();
        const auto update = mWaterSoundUpdater.update(player, *world);

        WaterSoundAction action;
        Sound_Buffer* sfx;
        std::tie(action, sfx) = getWaterSoundAction(update, curcell);

        switch (action)
        {
            case WaterSoundAction::DoNothing:
                break;
            case WaterSoundAction::SetVolume:
                mNearWaterSound->setVolume(update.mVolume * sfx->getVolume());
                mNearWaterSound->setFade(sSfxFadeInDuration, 1.0f, Play_FadeExponential);
                break;
            case WaterSoundAction::FinishSound:
                mNearWaterSound->setFade(sSfxFadeOutDuration, 0.0f, Play_FadeExponential | Play_StopAtFadeEnd);
                break;
            case WaterSoundAction::PlaySound:
                if (mNearWaterSound)
                    mOutput->finishSound(mNearWaterSound);
                mNearWaterSound = playSound(update.mId, update.mVolume, 1.0f, Type::Sfx, PlayMode::Loop);
                break;
        }

        mLastCell = curcell;
    }

    std::pair<SoundManager::WaterSoundAction, Sound_Buffer*> SoundManager::getWaterSoundAction(
            const WaterSoundUpdate& update, const ESM::Cell* cell) const
    {
        if (mNearWaterSound)
        {
            if (update.mVolume == 0.0f)
                return {WaterSoundAction::FinishSound, nullptr};

            bool soundIdChanged = false;

            Sound_Buffer* sfx = mSoundBuffers.lookup(update.mId);
            if (mLastCell != cell)
            {
                const auto snditer = mActiveSounds.find(nullptr);
                if (snditer != mActiveSounds.end())
                {
                    const auto pairiter = std::find_if(
                        snditer->second.mList.begin(), snditer->second.mList.end(),
                        [this](const SoundBufferRefPairList::value_type &item) -> bool
                        { return mNearWaterSound == item.first.get(); }
                    );
                    if (pairiter != snditer->second.mList.end() && pairiter->second != sfx)
                        soundIdChanged = true;
                }
            }

            if (soundIdChanged)
                return {WaterSoundAction::PlaySound, nullptr};

            if (sfx)
                return {WaterSoundAction::SetVolume, sfx};
        }
        else if (update.mVolume > 0.0f)
            return {WaterSoundAction::PlaySound, nullptr};

        return {WaterSoundAction::DoNothing, nullptr};
    }

    void SoundManager::cull3DSound(SoundBase *sound)
    {
        // Hard-coded distance of 2000.0f is from vanilla Morrowind
        const float maxDist = sound->getDistanceCull() ? 2000.0f : sound->getMaxDistance();
        const float squaredMaxDist = maxDist * maxDist;

        const osg::Vec3f pos = sound->getPosition();
        const float squaredDist = (mListenerPos - pos).length2();

        if (squaredDist > squaredMaxDist)
        {
            // If getDistanceCull() is set, delete the sound after it has faded out
            sound->setFade(sSfxFadeOutDuration, 0.0f, Play_FadeExponential | (sound->getDistanceCull() ? Play_StopAtFadeEnd : 0));
        }
        else
        {
            // Fade sounds back in once they are in range
            sound->setFade(sSfxFadeInDuration, 1.0f, Play_FadeExponential);
        }
    }


    void SoundManager::updateSounds(float duration)
    {
        // We update active say sounds map for specific actors here
        // because for vanilla compatibility we can't do it immediately.
        SaySoundMap::iterator queuesayiter = mSaySoundsQueue.begin();
        while (queuesayiter != mSaySoundsQueue.end())
        {
            const auto dst = mActiveSaySounds.find(queuesayiter->first);
            if (dst == mActiveSaySounds.end())
                mActiveSaySounds.emplace(queuesayiter->first, std::move(queuesayiter->second));
            else
                dst->second = std::move(queuesayiter->second);
            mSaySoundsQueue.erase(queuesayiter++);
        }

        mTimePassed += duration;
        if (mTimePassed < sMinUpdateInterval)
            return;
        duration = mTimePassed;
        mTimePassed = 0.0f;

        // Make sure music is still playing
        if(!isMusicPlaying() && !mCurrentPlaylist.empty())
            startRandomTitle();

        Environment env = Env_Normal;
        if (mListenerUnderwater)
            env = Env_Underwater;
        else if(mUnderwaterSound)
        {
            mOutput->finishSound(mUnderwaterSound);
            mUnderwaterSound = nullptr;
        }

        mOutput->startUpdate();
        mOutput->updateListener(
            mListenerPos,
            mListenerDir,
            mListenerUp,
            env
        );

        updateMusic(duration);

        // Check if any sounds are finished playing, and trash them
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            MWWorld::ConstPtr ptr = snditer->first;
            SoundBufferRefPairList::iterator sndidx = snditer->second.mList.begin();
            while(sndidx != snditer->second.mList.end())
            {
                Sound *sound = sndidx->first.get();

                if (sound->getIs3D())
                {
                    if (!ptr.isEmpty())
                        sound->setPosition(ptr.getRefData().getPosition().asVec3());

                    cull3DSound(sound);
                }

                if(!sound->updateFade(duration) || !mOutput->isSoundPlaying(sound))
                {
                    mOutput->finishSound(sound);
                    if (sound == mUnderwaterSound)
                        mUnderwaterSound = nullptr;
                    if (sound == mNearWaterSound)
                        mNearWaterSound = nullptr;
                    mSoundBuffers.release(*sndidx->second);
                    sndidx = snditer->second.mList.erase(sndidx);
                }
                else
                {
                    mOutput->updateSound(sound);
                    ++sndidx;
                }
            }
            if(snditer->second.mList.empty())
                snditer = mActiveSounds.erase(snditer);
            else
                ++snditer;
        }

        SaySoundMap::iterator sayiter = mActiveSaySounds.begin();
        while(sayiter != mActiveSaySounds.end())
        {
            MWWorld::ConstPtr ptr = sayiter->first;
            Stream *sound = sayiter->second.mStream.get();
            if (sound->getIs3D())
            {
                if (!ptr.isEmpty())
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    sound->setPosition(world->getActorHeadTransform(ptr).getTrans());
                }

                cull3DSound(sound);
            }

            if(!sound->updateFade(duration) || !mOutput->isStreamPlaying(sound))
            {
                mOutput->finishStream(sound);
                sayiter = mActiveSaySounds.erase(sayiter);
            }
            else
            {
                mOutput->updateStream(sound);
                ++sayiter;
            }
        }

        TrackList::iterator trkiter = mActiveTracks.begin();
        for(;trkiter != mActiveTracks.end();++trkiter)
        {
            Stream *sound = trkiter->get();
            if(!mOutput->isStreamPlaying(sound))
            {
                mOutput->finishStream(sound);
                trkiter = mActiveTracks.erase(trkiter);
            }
            else
            {
                sound->updateFade(duration);

                mOutput->updateStream(sound);
                ++trkiter;
            }
        }

        if(mListenerUnderwater)
        {
            // Play underwater sound (after updating sounds)
            if(!mUnderwaterSound)
                mUnderwaterSound = playSound("Underwater", 1.0f, 1.0f, Type::Sfx, PlayMode::LoopNoEnv);
        }
        mOutput->finishUpdate();
    }


    void SoundManager::updateMusic(float duration)
    {
        if (!mNextMusic.empty())
        {
            mMusic->updateFade(duration);

            mOutput->updateStream(mMusic.get());

            if (mMusic->getRealVolume() <= 0.f)
            {
                streamMusicFull(mNextMusic);
                mNextMusic.clear();
            }
        }
    }


    void SoundManager::update(float duration)
    {
        if(!mOutput->isInitialized() || mPlaybackPaused)
            return;

        updateSounds(duration);
        if (MWBase::Environment::get().getStateManager()->getState()!=
            MWBase::StateManager::State_NoGame)
        {
            updateRegionSound(duration);
            updateWaterSound();
        }
    }


    void SoundManager::processChangedSettings(const Settings::CategorySettingVector& settings)
    {
        mVolumeSettings.update();

        if(!mOutput->isInitialized())
            return;
        mOutput->startUpdate();
        for(SoundMap::value_type &snd : mActiveSounds)
        {
            for (SoundBufferRefPair& sndbuf : snd.second.mList)
            {
                Sound *sound = sndbuf.first.get();
                sound->setBaseVolume(volumeFromType(sound->getPlayType()));
                mOutput->updateSound(sound);
            }
        }
        for(SaySoundMap::value_type &snd : mActiveSaySounds)
        {
            Stream *sound = snd.second.mStream.get();
            sound->setBaseVolume(volumeFromType(sound->getPlayType()));
            mOutput->updateStream(sound);
        }
        for(SaySoundMap::value_type &snd : mSaySoundsQueue)
        {
            Stream *sound = snd.second.mStream.get();
            sound->setBaseVolume(volumeFromType(sound->getPlayType()));
            mOutput->updateStream(sound);
        }
        for (const StreamPtr& sound : mActiveTracks)
        {
            sound->setBaseVolume(volumeFromType(sound->getPlayType()));
            mOutput->updateStream(sound.get());
        }
        if(mMusic)
        {
            mMusic->setBaseVolume(volumeFromType(mMusic->getPlayType()));
            mOutput->updateStream(mMusic.get());
        }
        mOutput->finishUpdate();
    }

    void SoundManager::setListenerPosDir(const osg::Vec3f &pos, const osg::Vec3f &dir, const osg::Vec3f &up, bool underwater)
    {
        mListenerPos = pos;
        mListenerDir = dir;
        mListenerUp  = up;

        mListenerUnderwater = underwater;

        mWaterSoundUpdater.setUnderwater(underwater);
    }

    void SoundManager::updatePtr(const MWWorld::ConstPtr &old, const MWWorld::ConstPtr &updated)
    {
        SoundMap::iterator snditer = mActiveSounds.find(old.mRef);
        if(snditer != mActiveSounds.end())
            snditer->second.mCell = updated.mCell;

        if (const auto it = mSaySoundsQueue.find(old.mRef); it != mSaySoundsQueue.end())
            it->second.mCell = updated.mCell;

        if (const auto it = mActiveSaySounds.find(old.mRef); it != mActiveSaySounds.end())
            it->second.mCell = updated.mCell;
    }

    // Default readAll implementation, for decoders that can't do anything
    // better
    void Sound_Decoder::readAll(std::vector<char> &output)
    {
        size_t total = output.size();
        size_t got;

        output.resize(total+32768);
        while((got=read(&output[total], output.size()-total)) > 0)
        {
            total += got;
            output.resize(total*2);
        }
        output.resize(total);
    }


    const char *getSampleTypeName(SampleType type)
    {
        switch(type)
        {
            case SampleType_UInt8: return "U8";
            case SampleType_Int16: return "S16";
            case SampleType_Float32: return "Float32";
        }
        return "(unknown sample type)";
    }

    const char *getChannelConfigName(ChannelConfig config)
    {
        switch(config)
        {
            case ChannelConfig_Mono:    return "Mono";
            case ChannelConfig_Stereo:  return "Stereo";
            case ChannelConfig_Quad:    return "Quad";
            case ChannelConfig_5point1: return "5.1 Surround";
            case ChannelConfig_7point1: return "7.1 Surround";
        }
        return "(unknown channel config)";
    }

    size_t framesToBytes(size_t frames, ChannelConfig config, SampleType type)
    {
        switch(config)
        {
            case ChannelConfig_Mono:    frames *= 1; break;
            case ChannelConfig_Stereo:  frames *= 2; break;
            case ChannelConfig_Quad:    frames *= 4; break;
            case ChannelConfig_5point1: frames *= 6; break;
            case ChannelConfig_7point1: frames *= 8; break;
        }
        switch(type)
        {
            case SampleType_UInt8: frames *= 1; break;
            case SampleType_Int16: frames *= 2; break;
            case SampleType_Float32: frames *= 4; break;
        }
        return frames;
    }

    size_t bytesToFrames(size_t bytes, ChannelConfig config, SampleType type)
    {
        return bytes / framesToBytes(1, config, type);
    }

    void SoundManager::clear()
    {
        SoundManager::stopMusic();

        for(SoundMap::value_type &snd : mActiveSounds)
        {
            for (SoundBufferRefPair &sndbuf : snd.second.mList)
            {
                mOutput->finishSound(sndbuf.first.get());
                mSoundBuffers.release(*sndbuf.second);
            }
        }
        mActiveSounds.clear();
        mUnderwaterSound = nullptr;
        mNearWaterSound = nullptr;

        for(SaySoundMap::value_type &snd : mSaySoundsQueue)
            mOutput->finishStream(snd.second.mStream.get());
        mSaySoundsQueue.clear();

        for(SaySoundMap::value_type &snd : mActiveSaySounds)
            mOutput->finishStream(snd.second.mStream.get());
        mActiveSaySounds.clear();

        for(StreamPtr& sound : mActiveTracks)
            mOutput->finishStream(sound.get());
        mActiveTracks.clear();
        mPlaybackPaused = false;
        std::fill(std::begin(mPausedSoundTypes), std::end(mPausedSoundTypes), 0);
    }
}

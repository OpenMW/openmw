#include "soundmanagerimp.hpp"

#include <algorithm>
#include <map>
#include <numeric>

#include <osg/Matrixf>

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
    // For combining PlayMode and Type flags
    inline int operator|(PlayMode a, Type b) { return static_cast<int>(a) | static_cast<int>(b); }

    SoundManager::SoundManager(const VFS::Manager* vfs, bool useSound)
        : mVFS(vfs)
        , mOutput(new DEFAULT_OUTPUT(*this))
        , mMasterVolume(1.0f)
        , mSFXVolume(1.0f)
        , mMusicVolume(1.0f)
        , mVoiceVolume(1.0f)
        , mFootstepsVolume(1.0f)
        , mSoundBuffers(new SoundBufferList::element_type())
        , mBufferCacheSize(0)
        , mSounds(new std::deque<Sound>())
        , mStreams(new std::deque<Stream>())
        , mMusic(nullptr)
        , mListenerUnderwater(false)
        , mListenerPos(0,0,0)
        , mListenerDir(1,0,0)
        , mListenerUp(0,0,1)
        , mUnderwaterSound(nullptr)
        , mNearWaterSound(nullptr)
        , mPlaybackPaused(false)
    {
        mMasterVolume = Settings::Manager::getFloat("master volume", "Sound");
        mMasterVolume = std::min(std::max(mMasterVolume, 0.0f), 1.0f);
        mSFXVolume = Settings::Manager::getFloat("sfx volume", "Sound");
        mSFXVolume = std::min(std::max(mSFXVolume, 0.0f), 1.0f);
        mMusicVolume = Settings::Manager::getFloat("music volume", "Sound");
        mMusicVolume = std::min(std::max(mMusicVolume, 0.0f), 1.0f);
        mVoiceVolume = Settings::Manager::getFloat("voice volume", "Sound");
        mVoiceVolume = std::min(std::max(mVoiceVolume, 0.0f), 1.0f);
        mFootstepsVolume = Settings::Manager::getFloat("footsteps volume", "Sound");
        mFootstepsVolume = std::min(std::max(mFootstepsVolume, 0.0f), 1.0f);

        mNearWaterRadius = Fallback::Map::getInt("Water_NearWaterRadius");
        mNearWaterPoints = Fallback::Map::getInt("Water_NearWaterPoints");
        mNearWaterIndoorTolerance = Fallback::Map::getFloat("Water_NearWaterIndoorTolerance");
        mNearWaterOutdoorTolerance = Fallback::Map::getFloat("Water_NearWaterOutdoorTolerance");
        mNearWaterIndoorID = Misc::StringUtils::lowerCase(Fallback::Map::getString("Water_NearWaterIndoorID"));
        mNearWaterOutdoorID = Misc::StringUtils::lowerCase(Fallback::Map::getString("Water_NearWaterOutdoorID"));

        mBufferCacheMin = std::max(Settings::Manager::getInt("buffer cache min", "Sound"), 1);
        mBufferCacheMax = std::max(Settings::Manager::getInt("buffer cache max", "Sound"), 1);
        mBufferCacheMax *= 1024*1024;
        mBufferCacheMin = std::min(mBufferCacheMin*1024*1024, mBufferCacheMax);

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
        clear();
        for(Sound_Buffer &sfx : *mSoundBuffers)
        {
            if(sfx.mHandle)
                mOutput->unloadSound(sfx.mHandle);
            sfx.mHandle = 0;
        }
        mUnusedBuffers.clear();
        mOutput.reset();
    }

    // Return a new decoder instance, used as needed by the output implementations
    DecoderPtr SoundManager::getDecoder()
    {
        return DecoderPtr(new DEFAULT_DECODER (mVFS));
    }

    Sound_Buffer *SoundManager::insertSound(const std::string &soundId, const ESM::Sound *sound)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        static const float fAudioDefaultMinDistance = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMinDistance")->mValue.getFloat();
        static const float fAudioDefaultMaxDistance = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMaxDistance")->mValue.getFloat();
        static const float fAudioMinDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMinDistanceMult")->mValue.getFloat();
        static const float fAudioMaxDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMaxDistanceMult")->mValue.getFloat();
        float volume, min, max;

        volume = static_cast<float>(pow(10.0, (sound->mData.mVolume / 255.0*3348.0 - 3348.0) / 2000.0));
        min = sound->mData.mMinRange;
        max = sound->mData.mMaxRange;
        if (min == 0)
            min = fAudioDefaultMinDistance;
        if (max == 0)
            max = fAudioDefaultMaxDistance;

        min *= fAudioMinDistanceMult;
        max *= fAudioMaxDistanceMult;
        min = std::max(min, 1.0f);
        max = std::max(min, max);

        Sound_Buffer *sfx = &*mSoundBuffers->insert(mSoundBuffers->end(),
            Sound_Buffer("Sound/"+sound->mSound, volume, min, max)
        );
        mVFS->normalizeFilename(sfx->mResourceName);

        mBufferNameMap.insert(std::make_pair(soundId, sfx));

        return sfx;
    }

    // Lookup a soundId for its sound data (resource name, local volume,
    // minRange, and maxRange)
    Sound_Buffer *SoundManager::lookupSound(const std::string &soundId) const
    {
        NameBufferMap::const_iterator snd = mBufferNameMap.find(soundId);
        if(snd != mBufferNameMap.end())
        {
            Sound_Buffer *sfx = snd->second;
            if(sfx->mHandle) return sfx;
        }
        return nullptr;
    }

    // Lookup a soundId for its sound data (resource name, local volume,
    // minRange, and maxRange), and ensure it's ready for use.
    Sound_Buffer *SoundManager::loadSound(const std::string &soundId)
    {
#ifdef __GNUC__
#define LIKELY(x) __builtin_expect((bool)(x), true)
#define UNLIKELY(x) __builtin_expect((bool)(x), false)
#else
#define LIKELY(x) (bool)(x)
#define UNLIKELY(x) (bool)(x)
#endif
        if(UNLIKELY(mBufferNameMap.empty()))
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            for(const ESM::Sound &sound : world->getStore().get<ESM::Sound>())
                insertSound(Misc::StringUtils::lowerCase(sound.mId), &sound);
        }

        Sound_Buffer *sfx;
        NameBufferMap::const_iterator snd = mBufferNameMap.find(soundId);
        if(LIKELY(snd != mBufferNameMap.end()))
            sfx = snd->second;
        else
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            const ESM::Sound *sound = world->getStore().get<ESM::Sound>().search(soundId);
            if(!sound) return nullptr;
            sfx = insertSound(soundId, sound);
        }
#undef LIKELY
#undef UNLIKELY

        if(!sfx->mHandle)
        {
            size_t size;
            std::tie(sfx->mHandle, size) = mOutput->loadSound(sfx->mResourceName);
            if(!sfx->mHandle) return nullptr;

            mBufferCacheSize += size;
            if(mBufferCacheSize > mBufferCacheMax)
            {
                do {
                    if(mUnusedBuffers.empty())
                    {
                        Log(Debug::Warning) << "No unused sound buffers to free, using " << mBufferCacheSize << " bytes!";
                        break;
                    }
                    Sound_Buffer *unused = mUnusedBuffers.back();

                    size = mOutput->unloadSound(unused->mHandle);
                    mBufferCacheSize -= size;
                    unused->mHandle = 0;

                    mUnusedBuffers.pop_back();
                } while(mBufferCacheSize > mBufferCacheMin);
            }
            mUnusedBuffers.push_front(sfx);
        }

        return sfx;
    }

    DecoderPtr SoundManager::loadVoice(const std::string &voicefile)
    {
        try
        {
            DecoderPtr decoder = getDecoder();

            // Workaround: Bethesda at some point converted some of the files to mp3, but the references were kept as .wav.
            if(mVFS->exists(voicefile))
                decoder->open(voicefile);
            else
            {
                std::string file = voicefile;
                std::string::size_type pos = file.rfind('.');
                if(pos != std::string::npos)
                    file = file.substr(0, pos)+".mp3";
                decoder->open(file);
            }

            return decoder;
        }
        catch(std::exception &e)
        {
            Log(Debug::Error) << "Failed to load audio from " << voicefile << ": " << e.what();
        }

        return nullptr;
    }

    Sound *SoundManager::getSoundRef()
    {
        Sound *ret;
        if(!mUnusedSounds.empty())
        {
            ret = mUnusedSounds.back();
            mUnusedSounds.pop_back();
        }
        else
        {
            mSounds->emplace_back();
            ret = &mSounds->back();
        }
        return ret;
    }

    Stream *SoundManager::getStreamRef()
    {
        Stream *ret;
        if(!mUnusedStreams.empty())
        {
            ret = mUnusedStreams.back();
            mUnusedStreams.pop_back();
        }
        else
        {
            mStreams->emplace_back();
            ret = &mStreams->back();
        }
        return ret;
    }

    Stream *SoundManager::playVoice(DecoderPtr decoder, const osg::Vec3f &pos, bool playlocal)
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
        Stream *sound = getStreamRef();
        if(playlocal)
        {
            sound->init(1.0f, basevol, 1.0f, PlayMode::NoEnv|Type::Voice|Play_2D);
            played = mOutput->streamSound(decoder, sound, true);
        }
        else
        {
            sound->init(pos, 1.0f, basevol, 1.0f, minDistance, maxDistance,
                        PlayMode::Normal|Type::Voice|Play_3D);
            played = mOutput->streamSound3D(decoder, sound, true);
        }
        if(!played)
        {
            mUnusedStreams.push_back(sound);
            return nullptr;
        }
        return sound;
    }

    // Gets the combined volume settings for the given sound type
    float SoundManager::volumeFromType(Type type) const
    {
        float volume = mMasterVolume;
        switch(type)
        {
            case Type::Sfx:
                volume *= mSFXVolume;
                break;
            case Type::Voice:
                volume *= mVoiceVolume;
                break;
            case Type::Foot:
                volume *= mFootstepsVolume;
                break;
            case Type::Music:
                volume *= mMusicVolume;
                break;
            case Type::Movie:
            case Type::Mask:
                break;
        }
        return volume;
    }

    void SoundManager::stopMusic()
    {
        if(mMusic)
        {
            mOutput->finishStream(mMusic);
            mUnusedStreams.push_back(mMusic);
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
        decoder->open(filename);

        mMusic = getStreamRef();
        mMusic->init(1.0f, volumeFromType(Type::Music), 1.0f,
                     PlayMode::NoEnv|Type::Music|Play_2D);
        mOutput->streamSound(decoder, mMusic);
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
        return mMusic && mOutput->isStreamPlaying(mMusic);
    }

    void SoundManager::playPlaylist(const std::string &playlist)
    {
        if (mCurrentPlaylist == playlist)
            return;

        if (mMusicFiles.find(playlist) == mMusicFiles.end())
        {
            std::vector<std::string> filelist;
            const std::map<std::string, VFS::File*>& index = mVFS->getIndex();

            std::string pattern = "Music/" + playlist;
            mVFS->normalizeFilename(pattern);

            std::map<std::string, VFS::File*>::const_iterator found = index.lower_bound(pattern);
            while (found != index.end())
            {
                if (found->first.size() >= pattern.size() && found->first.substr(0, pattern.size()) == pattern)
                    filelist.push_back(found->first);
                else
                    break;
                ++found;
            }

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
            const std::map<std::string, VFS::File*>& index = mVFS->getIndex();
            // Is there an ini setting for this filename or something?
            std::string filename = "music/special/morrowind title.mp3";
            auto found = index.find(filename);
            if (found != index.end())
            {
                filelist.emplace_back(found->first);
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

        std::string voicefile = "Sound/"+filename;

        mVFS->normalizeFilename(voicefile);
        DecoderPtr decoder = loadVoice(voicefile);
        if (!decoder)
            return;

        MWBase::World *world = MWBase::Environment::get().getWorld();
        const osg::Vec3f pos = world->getActorHeadTransform(ptr).getTrans();

        stopSay(ptr);
        Stream *sound = playVoice(decoder, pos, (ptr == MWMechanics::getPlayer()));
        if(!sound) return;

        mSaySoundsQueue.emplace(ptr, sound);
    }

    float SoundManager::getSaySoundLoudness(const MWWorld::ConstPtr &ptr) const
    {
        SaySoundMap::const_iterator snditer = mActiveSaySounds.find(ptr);
        if(snditer != mActiveSaySounds.end())
        {
            Stream *sound = snditer->second;
            return mOutput->getStreamLoudness(sound);
        }

        return 0.0f;
    }

    void SoundManager::say(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;

        std::string voicefile = "Sound/"+filename;

        mVFS->normalizeFilename(voicefile);
        DecoderPtr decoder = loadVoice(voicefile);
        if (!decoder)
            return;

        stopSay(MWWorld::ConstPtr());
        Stream *sound = playVoice(decoder, osg::Vec3f(), true);
        if(!sound) return;

        mActiveSaySounds.insert(std::make_pair(MWWorld::ConstPtr(), sound));
    }

    bool SoundManager::sayDone(const MWWorld::ConstPtr &ptr) const
    {
        SaySoundMap::const_iterator snditer = mActiveSaySounds.find(ptr);
        if(snditer != mActiveSaySounds.end())
        {
            if(mOutput->isStreamPlaying(snditer->second))
                return false;
            return true;
        }
        return true;
    }

    bool SoundManager::sayActive(const MWWorld::ConstPtr &ptr) const
    {
        SaySoundMap::const_iterator snditer = mSaySoundsQueue.find(ptr);
        if(snditer != mSaySoundsQueue.end())
        {
            if(mOutput->isStreamPlaying(snditer->second))
                return true;
            return false;
        }

        snditer = mActiveSaySounds.find(ptr);
        if(snditer != mActiveSaySounds.end())
        {
            if(mOutput->isStreamPlaying(snditer->second))
                return true;
            return false;
        }

        return false;
    }

    void SoundManager::stopSay(const MWWorld::ConstPtr &ptr)
    {
        SaySoundMap::iterator snditer = mSaySoundsQueue.find(ptr);
        if(snditer != mSaySoundsQueue.end())
        {
            mOutput->finishStream(snditer->second);
            mUnusedStreams.push_back(snditer->second);
            mSaySoundsQueue.erase(snditer);
        }

        snditer = mActiveSaySounds.find(ptr);
        if(snditer != mActiveSaySounds.end())
        {
            mOutput->finishStream(snditer->second);
            mUnusedStreams.push_back(snditer->second);
            mActiveSaySounds.erase(snditer);
        }
    }


    Stream *SoundManager::playTrack(const DecoderPtr& decoder, Type type)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        Stream *track = getStreamRef();
        track->init(1.0f, volumeFromType(type), 1.0f, PlayMode::NoEnv|type|Play_2D);
        if(!mOutput->streamSound(decoder, track))
        {
            mUnusedStreams.push_back(track);
            return nullptr;
        }

        mActiveTracks.insert(
            std::lower_bound(mActiveTracks.begin(), mActiveTracks.end(), track), track
        );
        return track;
    }

    void SoundManager::stopTrack(Stream *stream)
    {
        mOutput->finishStream(stream);
        TrackList::iterator iter = std::lower_bound(mActiveTracks.begin(), mActiveTracks.end(), stream);
        if(iter != mActiveTracks.end() && *iter == stream)
            mActiveTracks.erase(iter);
        mUnusedStreams.push_back(stream);
    }

    double SoundManager::getTrackTimeDelay(Stream *stream)
    {
        return mOutput->getStreamDelay(stream);
    }


    Sound *SoundManager::playSound(const std::string& soundId, float volume, float pitch, Type type, PlayMode mode, float offset)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        Sound_Buffer *sfx = loadSound(Misc::StringUtils::lowerCase(soundId));
        if(!sfx) return nullptr;

        // Only one copy of given sound can be played at time, so stop previous copy
        stopSound(sfx, MWWorld::ConstPtr());

        Sound *sound = getSoundRef();
        sound->init(volume * sfx->mVolume, volumeFromType(type), pitch, mode|type|Play_2D);
        if(!mOutput->playSound(sound, sfx->mHandle, offset))
        {
            mUnusedSounds.push_back(sound);
            return nullptr;
        }

        if(sfx->mUses++ == 0)
        {
            SoundList::iterator iter = std::find(mUnusedBuffers.begin(), mUnusedBuffers.end(), sfx);
            if(iter != mUnusedBuffers.end())
                mUnusedBuffers.erase(iter);
        }
        mActiveSounds[MWWorld::ConstPtr()].push_back(std::make_pair(sound, sfx));
        return sound;
    }

    Sound *SoundManager::playSound3D(const MWWorld::ConstPtr &ptr, const std::string& soundId,
                                     float volume, float pitch, Type type, PlayMode mode,
                                     float offset)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        // Look up the sound in the ESM data
        Sound_Buffer *sfx = loadSound(Misc::StringUtils::lowerCase(soundId));
        if(!sfx) return nullptr;

        const osg::Vec3f objpos(ptr.getRefData().getPosition().asVec3());
        if((mode&PlayMode::RemoveAtDistance) && (mListenerPos-objpos).length2() > 2000*2000)
            return nullptr;

        // Only one copy of given sound can be played at time on ptr, so stop previous copy
        stopSound(sfx, ptr);

        bool played;
        Sound *sound = getSoundRef();
        if(!(mode&PlayMode::NoPlayerLocal) && ptr == MWMechanics::getPlayer())
        {
            sound->init(volume * sfx->mVolume, volumeFromType(type), pitch, mode|type|Play_2D);
            played = mOutput->playSound(sound, sfx->mHandle, offset);
        }
        else
        {
            sound->init(objpos, volume * sfx->mVolume, volumeFromType(type), pitch,
                        sfx->mMinDist, sfx->mMaxDist, mode|type|Play_3D);
            played = mOutput->playSound3D(sound, sfx->mHandle, offset);
        }
        if(!played)
        {
            mUnusedSounds.push_back(sound);
            return nullptr;
        }

        if(sfx->mUses++ == 0)
        {
            SoundList::iterator iter = std::find(mUnusedBuffers.begin(), mUnusedBuffers.end(), sfx);
            if(iter != mUnusedBuffers.end())
                mUnusedBuffers.erase(iter);
        }
        mActiveSounds[ptr].push_back(std::make_pair(sound, sfx));
        return sound;
    }

    Sound *SoundManager::playSound3D(const osg::Vec3f& initialPos, const std::string& soundId,
                                     float volume, float pitch, Type type, PlayMode mode,
                                     float offset)
    {
        if(!mOutput->isInitialized())
            return nullptr;

        // Look up the sound in the ESM data
        Sound_Buffer *sfx = loadSound(Misc::StringUtils::lowerCase(soundId));
        if(!sfx) return nullptr;

        Sound *sound = getSoundRef();
        sound->init(initialPos, volume * sfx->mVolume, volumeFromType(type), pitch,
                    sfx->mMinDist, sfx->mMaxDist, mode|type|Play_3D);
        if(!mOutput->playSound3D(sound, sfx->mHandle, offset))
        {
            mUnusedSounds.push_back(sound);
            return nullptr;
        }

        if(sfx->mUses++ == 0)
        {
            SoundList::iterator iter = std::find(mUnusedBuffers.begin(), mUnusedBuffers.end(), sfx);
            if(iter != mUnusedBuffers.end())
                mUnusedBuffers.erase(iter);
        }
        mActiveSounds[MWWorld::ConstPtr()].push_back(std::make_pair(sound, sfx));
        return sound;
    }

    void SoundManager::stopSound(Sound *sound)
    {
        if(sound)
            mOutput->finishSound(sound);
    }

    void SoundManager::stopSound(Sound_Buffer *sfx, const MWWorld::ConstPtr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.find(ptr);
        if(snditer != mActiveSounds.end())
        {
            for(SoundBufferRefPair &snd : snditer->second)
            {
                if(snd.second == sfx)
                    mOutput->finishSound(snd.first);
            }
        }
    }

    void SoundManager::stopSound(const std::string& soundId)
    {
        if(!mOutput->isInitialized())
            return;

        Sound_Buffer *sfx = loadSound(Misc::StringUtils::lowerCase(soundId));
        if (!sfx) return;

        stopSound(sfx, MWWorld::ConstPtr());
    }

    void SoundManager::stopSound3D(const MWWorld::ConstPtr &ptr, const std::string& soundId)
    {
        if(!mOutput->isInitialized())
            return;

        Sound_Buffer *sfx = loadSound(Misc::StringUtils::lowerCase(soundId));
        if (!sfx) return;

        stopSound(sfx, ptr);
    }

    void SoundManager::stopSound3D(const MWWorld::ConstPtr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.find(ptr);
        if(snditer != mActiveSounds.end())
        {
            for(SoundBufferRefPair &snd : snditer->second)
                mOutput->finishSound(snd.first);
        }
        SaySoundMap::iterator sayiter = mSaySoundsQueue.find(ptr);
        if(sayiter != mSaySoundsQueue.end())
            mOutput->finishStream(sayiter->second);
        sayiter = mActiveSaySounds.find(ptr);
        if(sayiter != mActiveSaySounds.end())
            mOutput->finishStream(sayiter->second);
    }

    void SoundManager::stopSound(const MWWorld::CellStore *cell)
    {
        for(SoundMap::value_type &snd : mActiveSounds)
        {
            if(!snd.first.isEmpty() && snd.first != MWMechanics::getPlayer() && snd.first.getCell() == cell)
            {
                for(SoundBufferRefPair &sndbuf : snd.second)
                    mOutput->finishSound(sndbuf.first);
            }
        }

        for(SaySoundMap::value_type &snd : mSaySoundsQueue)
        {
            if(!snd.first.isEmpty() && snd.first != MWMechanics::getPlayer() && snd.first.getCell() == cell)
                mOutput->finishStream(snd.second);
        }

        for(SaySoundMap::value_type &snd : mActiveSaySounds)
        {
            if(!snd.first.isEmpty() && snd.first != MWMechanics::getPlayer() && snd.first.getCell() == cell)
                mOutput->finishStream(snd.second);
        }
    }

    void SoundManager::fadeOutSound3D(const MWWorld::ConstPtr &ptr,
            const std::string& soundId, float duration)
    {
        SoundMap::iterator snditer = mActiveSounds.find(ptr);
        if(snditer != mActiveSounds.end())
        {
            Sound_Buffer *sfx = loadSound(Misc::StringUtils::lowerCase(soundId));
            for(SoundBufferRefPair &sndbuf : snditer->second)
            {
                if(sndbuf.second == sfx)
                    sndbuf.first->setFadeout(duration);
            }
        }
    }

    bool SoundManager::getSoundPlaying(const MWWorld::ConstPtr &ptr, const std::string& soundId) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.find(ptr);
        if(snditer != mActiveSounds.end())
        {
            Sound_Buffer *sfx = lookupSound(Misc::StringUtils::lowerCase(soundId));
            return std::find_if(snditer->second.cbegin(), snditer->second.cend(),
                [this,sfx](const SoundBufferRefPair &snd) -> bool
                { return snd.second == sfx && mOutput->isSoundPlaying(snd.first); }
            ) != snditer->second.cend();
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
        static float sTimeToNextEnvSound = 0.0f;
        static int total = 0;
        static std::string regionName = "";
        static float sTimePassed = 0.0;
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::ConstPtr player = world->getPlayerPtr();
        const ESM::Cell *cell = player.getCell()->getCell();

        sTimePassed += duration;
        if(!cell->isExterior() || sTimePassed < sTimeToNextEnvSound)
            return;

        float a = Misc::Rng::rollClosedProbability();
        // NOTE: We should use the "Minimum Time Between Environmental Sounds" and
        // "Maximum Time Between Environmental Sounds" fallback settings here.
        sTimeToNextEnvSound = 5.0f*a + 15.0f*(1.0f-a);
        sTimePassed = 0;

        if(regionName != cell->mRegion)
        {
            regionName = cell->mRegion;
            total = 0;
        }

        const ESM::Region *regn = world->getStore().get<ESM::Region>().search(regionName);
        if(regn == nullptr)
            return;

        if(total == 0)
        {
            for(const ESM::Region::SoundRef &sndref : regn->mSoundList)
                total += (int)sndref.mChance;
            if(total == 0)
                return;
        }

        int r = Misc::Rng::rollDice(total);
        int pos = 0;

        for(const ESM::Region::SoundRef &sndref : regn->mSoundList)
        {
            if(r - pos < sndref.mChance)
            {
                playSound(sndref.mSound, 1.0f, 1.0f);
                break;
            }
            pos += sndref.mChance;
        }
    }

    void SoundManager::updateWaterSound(float /*duration*/)
    {
        static const ESM::Cell *LastCell;
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::ConstPtr player = world->getPlayerPtr();
        osg::Vec3f pos = player.getRefData().getPosition().asVec3();
        const ESM::Cell *curcell = player.getCell()->getCell();

        float volume = 0.0f;
        const std::string& soundId = player.getCell()->isExterior() ? mNearWaterOutdoorID : mNearWaterIndoorID;

        if (!mListenerUnderwater)
        {
            if (curcell->hasWater())
            {
                float dist = std::abs(player.getCell()->getWaterLevel() - pos.z());

                if (player.getCell()->isExterior() && dist < mNearWaterOutdoorTolerance)
                {
                    volume = (mNearWaterOutdoorTolerance - dist) / mNearWaterOutdoorTolerance;

                    if (mNearWaterPoints > 1)
                    {
                        int underwaterPoints = 0;

                        float step = mNearWaterRadius * 2.0f / (mNearWaterPoints - 1);

                        for (int x = 0; x < mNearWaterPoints; x++)
                        {
                            for (int y = 0; y < mNearWaterPoints; y++)
                            {
                                float height = world->getTerrainHeightAt(
                                            osg::Vec3f(pos.x() - mNearWaterRadius + x*step, pos.y() - mNearWaterRadius + y*step, 0.0f));

                                if (height < 0)
                                    underwaterPoints++;
                            }
                        }

                        volume *= underwaterPoints * 2.0f / (mNearWaterPoints*mNearWaterPoints);
                    }
                }
                else if (!player.getCell()->isExterior() && dist < mNearWaterIndoorTolerance)
                {
                    volume = (mNearWaterIndoorTolerance - dist) / mNearWaterIndoorTolerance;
                }
            }
        }
        else
            volume = 1.0f;

        volume = std::min(volume, 1.0f);

        if (mNearWaterSound)
        {
            if (volume == 0.0f)
            {
                mOutput->finishSound(mNearWaterSound);
                mNearWaterSound = nullptr;
            }
            else
            {
                bool soundIdChanged = false;

                Sound_Buffer *sfx = lookupSound(soundId);
                if(LastCell != curcell)
                {
                    LastCell = curcell;
                    SoundMap::const_iterator snditer = mActiveSounds.find(MWWorld::Ptr());
                    if(snditer != mActiveSounds.end())
                    {
                        SoundBufferRefPairList::const_iterator pairiter = std::find_if(
                            snditer->second.begin(), snditer->second.end(),
                            [this](const SoundBufferRefPairList::value_type &item) -> bool
                            { return mNearWaterSound == item.first; }
                        );
                        if (pairiter != snditer->second.end() && pairiter->second != sfx)
                            soundIdChanged = true;
                    }
                }

                if(soundIdChanged)
                {
                    mOutput->finishSound(mNearWaterSound);
                    mNearWaterSound = playSound(soundId, volume, 1.0f, Type::Sfx, PlayMode::Loop);
                }
                else if (sfx)
                    mNearWaterSound->setVolume(volume * sfx->mVolume);
            }
        }
        else if (volume > 0.0f)
        {
            LastCell = curcell;
            mNearWaterSound = playSound(soundId, volume, 1.0f, Type::Sfx, PlayMode::Loop);
        }
    }

    void SoundManager::updateSounds(float duration)
    {
        // We update active say sounds map for specific actors here
        // because for vanilla compatibility we can't do it immediately.
        SaySoundMap::iterator queuesayiter = mSaySoundsQueue.begin();
        while (queuesayiter != mSaySoundsQueue.end())
        {
            mActiveSaySounds[queuesayiter->first] = queuesayiter->second;
            mSaySoundsQueue.erase(queuesayiter++);
        }

        static float timePassed = 0.0;

        timePassed += duration;
        if(timePassed < (1.0f/30.0f))
            return;
        duration = timePassed;
        timePassed = 0.0f;

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
            SoundBufferRefPairList::iterator sndidx = snditer->second.begin();
            while(sndidx != snditer->second.end())
            {
                Sound *sound;
                Sound_Buffer *sfx;

                std::tie(sound, sfx) = *sndidx;
                if(!ptr.isEmpty() && sound->getIs3D())
                {
                    const ESM::Position &pos = ptr.getRefData().getPosition();
                    const osg::Vec3f objpos(pos.asVec3());
                    sound->setPosition(objpos);

                    if(sound->getDistanceCull())
                    {
                        if((mListenerPos - objpos).length2() > 2000*2000)
                            mOutput->finishSound(sound);
                    }
                }

                if(!mOutput->isSoundPlaying(sound))
                {
                    mOutput->finishSound(sound);
                    mUnusedSounds.push_back(sound);
                    if(sound == mUnderwaterSound)
                        mUnderwaterSound = nullptr;
                    if(sound == mNearWaterSound)
                        mNearWaterSound = nullptr;
                    if(sfx->mUses-- == 1)
                        mUnusedBuffers.push_front(sfx);
                    sndidx = snditer->second.erase(sndidx);
                }
                else
                {
                    sound->updateFade(duration);

                    mOutput->updateSound(sound);
                    ++sndidx;
                }
            }
            if(snditer->second.empty())
                snditer = mActiveSounds.erase(snditer);
            else
                ++snditer;
        }

        SaySoundMap::iterator sayiter = mActiveSaySounds.begin();
        while(sayiter != mActiveSaySounds.end())
        {
            MWWorld::ConstPtr ptr = sayiter->first;
            Stream *sound = sayiter->second;
            if(!ptr.isEmpty() && sound->getIs3D())
            {
                MWBase::World *world = MWBase::Environment::get().getWorld();
                const osg::Vec3f pos = world->getActorHeadTransform(ptr).getTrans();
                sound->setPosition(pos);

                if(sound->getDistanceCull())
                {
                    if((mListenerPos - pos).length2() > 2000*2000)
                        mOutput->finishStream(sound);
                }
            }

            if(!mOutput->isStreamPlaying(sound))
            {
                mOutput->finishStream(sound);
                mUnusedStreams.push_back(sound);
                mActiveSaySounds.erase(sayiter++);
            }
            else
            {
                sound->updateFade(duration);

                mOutput->updateStream(sound);
                ++sayiter;
            }
        }

        TrackList::iterator trkiter = mActiveTracks.begin();
        for(;trkiter != mActiveTracks.end();++trkiter)
        {
            Stream *sound = *trkiter;
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

            mOutput->updateStream(mMusic);
            
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
            updateWaterSound(duration);
        }
    }


    void SoundManager::processChangedSettings(const Settings::CategorySettingVector& settings)
    {
        mMasterVolume = Settings::Manager::getFloat("master volume", "Sound");
        mMusicVolume = Settings::Manager::getFloat("music volume", "Sound");
        mSFXVolume = Settings::Manager::getFloat("sfx volume", "Sound");
        mFootstepsVolume = Settings::Manager::getFloat("footsteps volume", "Sound");
        mVoiceVolume = Settings::Manager::getFloat("voice volume", "Sound");

        if(!mOutput->isInitialized())
            return;
        mOutput->startUpdate();
        for(SoundMap::value_type &snd : mActiveSounds)
        {
            for(SoundBufferRefPair &sndbuf : snd.second)
            {
                Sound *sound = sndbuf.first;
                sound->setBaseVolume(volumeFromType(sound->getPlayType()));
                mOutput->updateSound(sound);
            }
        }
        for(SaySoundMap::value_type &snd : mActiveSaySounds)
        {
            Stream *sound = snd.second;
            sound->setBaseVolume(volumeFromType(sound->getPlayType()));
            mOutput->updateStream(sound);
        }
        for(SaySoundMap::value_type &snd : mSaySoundsQueue)
        {
            Stream *sound = snd.second;
            sound->setBaseVolume(volumeFromType(sound->getPlayType()));
            mOutput->updateStream(sound);
        }
        for(Stream *sound : mActiveTracks)
        {
            sound->setBaseVolume(volumeFromType(sound->getPlayType()));
            mOutput->updateStream(sound);
        }
        if(mMusic)
        {
            mMusic->setBaseVolume(volumeFromType(mMusic->getPlayType()));
            mOutput->updateStream(mMusic);
        }
        mOutput->finishUpdate();
    }

    void SoundManager::setListenerPosDir(const osg::Vec3f &pos, const osg::Vec3f &dir, const osg::Vec3f &up, bool underwater)
    {
        mListenerPos = pos;
        mListenerDir = dir;
        mListenerUp  = up;

        mListenerUnderwater = underwater;
    }

    void SoundManager::updatePtr(const MWWorld::ConstPtr &old, const MWWorld::ConstPtr &updated)
    {
        SoundMap::iterator snditer = mActiveSounds.find(old);
        if(snditer != mActiveSounds.end())
        {
            SoundBufferRefPairList sndlist = std::move(snditer->second);
            mActiveSounds.erase(snditer);
            mActiveSounds.emplace(updated, std::move(sndlist));
        }

        SaySoundMap::iterator sayiter = mSaySoundsQueue.find(old);
        if(sayiter != mSaySoundsQueue.end())
        {
            Stream *stream = sayiter->second;
            mSaySoundsQueue.erase(sayiter);
            mSaySoundsQueue.emplace(updated, stream);
        }

        sayiter = mActiveSaySounds.find(old);
        if(sayiter != mActiveSaySounds.end())
        {
            Stream *stream = sayiter->second;
            mActiveSaySounds.erase(sayiter);
            mActiveSaySounds.emplace(updated, stream);
        }
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
            for(SoundBufferRefPair &sndbuf : snd.second)
            {
                mOutput->finishSound(sndbuf.first);
                mUnusedSounds.push_back(sndbuf.first);
                Sound_Buffer *sfx = sndbuf.second;
                if(sfx->mUses-- == 1)
                    mUnusedBuffers.push_front(sfx);
            }
        }
        mActiveSounds.clear();
        mUnderwaterSound = nullptr;
        mNearWaterSound = nullptr;

        for(SaySoundMap::value_type &snd : mSaySoundsQueue)
        {
            mOutput->finishStream(snd.second);
            mUnusedStreams.push_back(snd.second);
        }
        mSaySoundsQueue.clear();

        for(SaySoundMap::value_type &snd : mActiveSaySounds)
        {
            mOutput->finishStream(snd.second);
            mUnusedStreams.push_back(snd.second);
        }
        mActiveSaySounds.clear();

        for(Stream *sound : mActiveTracks)
        {
            mOutput->finishStream(sound);
            mUnusedStreams.push_back(sound);
        }
        mActiveTracks.clear();
        mPlaybackPaused = false;
        std::fill(std::begin(mPausedSoundTypes), std::end(mPausedSoundTypes), 0);
    }
}

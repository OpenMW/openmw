#include "soundmanagerimp.hpp"

#include <iostream>
#include <algorithm>
#include <map>

#include <openengine/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "sound_output.hpp"
#include "sound_decoder.hpp"
#include "sound.hpp"

#include "openal_output.hpp"
#define SOUND_OUT "OpenAL"
#include "ffmpeg_decoder.hpp"
#ifndef SOUND_IN
#define SOUND_IN "FFmpeg"
#endif


namespace MWSound
{
    SoundManager::SoundManager(bool useSound)
        : mResourceMgr(Ogre::ResourceGroupManager::getSingleton())
        , mOutput(new DEFAULT_OUTPUT(*this))
        , mMasterVolume(1.0f)
        , mSFXVolume(1.0f)
        , mMusicVolume(1.0f)
        , mFootstepsVolume(1.0f)
        , mVoiceVolume(1.0f)
        , mPausedSoundTypes(0)
        , mListenerPos(0,0,0)
        , mListenerDir(1,0,0)
        , mListenerUp(0,0,1)
        , mListenerUnderwater(false)
    {
        if(!useSound)
            return;

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

        std::cout << "Sound output: " << SOUND_OUT << std::endl;
        std::cout << "Sound decoder: " << SOUND_IN << std::endl;

        try
        {
            std::vector<std::string> names = mOutput->enumerate();
            std::cout <<"Enumerated output devices:"<< std::endl;
            for(size_t i = 0;i < names.size();i++)
                std::cout <<"  "<<names[i]<< std::endl;

            std::string devname = Settings::Manager::getString("device", "Sound");
            try
            {
                mOutput->init(devname);
            }
            catch(std::exception &e)
            {
                if(devname.empty())
                    throw;
                std::cerr <<"Failed to open device \""<<devname<<"\": " << e.what() << std::endl;
                mOutput->init();
                Settings::Manager::setString("device", "Sound", "");
            }
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound init failed: "<<e.what()<< std::endl;
        }
    }

    SoundManager::~SoundManager()
    {
        mUnderwaterSound.reset();
        mActiveSounds.clear();
        mMusic.reset();
        mOutput.reset();
    }

    // Return a new decoder instance, used as needed by the output implementations
    DecoderPtr SoundManager::getDecoder()
    {
        return DecoderPtr(new DEFAULT_DECODER);
    }

    // Convert a soundId to file name, and modify the volume
    // according to the sounds local volume setting, minRange and
    // maxRange.
    std::string SoundManager::lookup(const std::string &soundId,
                       float &volume, float &min, float &max)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const ESM::Sound *snd = world->getStore().get<ESM::Sound>().find(soundId);

        volume *= static_cast<float>(pow(10.0, (snd->mData.mVolume / 255.0*3348.0 - 3348.0) / 2000.0));

        if(snd->mData.mMinRange == 0 && snd->mData.mMaxRange == 0)
        {
            static const float fAudioDefaultMinDistance = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMinDistance")->getFloat();
            static const float fAudioDefaultMaxDistance = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMaxDistance")->getFloat();
            min = fAudioDefaultMinDistance;
            max = fAudioDefaultMaxDistance;
        }
        else
        {
            min = snd->mData.mMinRange;
            max = snd->mData.mMaxRange;
        }

        static const float fAudioMinDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMinDistanceMult")->getFloat();
        static const float fAudioMaxDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMaxDistanceMult")->getFloat();
        min *= fAudioMinDistanceMult;
        max *= fAudioMaxDistanceMult;
        min = std::max(min, 1.0f);
        max = std::max(min, max);

        return "Sound/"+snd->mSound;
    }

    // Gets the combined volume settings for the given sound type
    float SoundManager::volumeFromType(PlayType type) const
    {
        float volume = mMasterVolume;
        switch(type)
        {
            case Play_TypeSfx:
                volume *= mSFXVolume;
                break;
            case Play_TypeVoice:
                volume *= mVoiceVolume;
                break;
            case Play_TypeFoot:
                volume *= mFootstepsVolume;
                break;
            case Play_TypeMusic:
                volume *= mMusicVolume;
                break;
            case Play_TypeMask:
                break;
            default:
                break;
        }
        return volume;
    }

    bool SoundManager::isPlaying(const MWWorld::Ptr &ptr, const std::string &id) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == id)
                return snditer->first->isPlaying();
            ++snditer;
        }
        return false;
    }


    void SoundManager::stopMusic()
    {
        if(mMusic)
            mMusic->stop();
        mMusic.reset();
    }

    void SoundManager::streamMusicFull(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        std::cout <<"Playing "<<filename<< std::endl;
        mLastPlayedMusic = filename;
        try
        {
            stopMusic();

            DecoderPtr decoder = getDecoder();
            decoder->open(filename);

            mMusic = mOutput->streamSound(decoder, volumeFromType(Play_TypeMusic),
                                          1.0f, Play_NoEnv|Play_TypeMusic);
        }
        catch(std::exception &e)
        {
            std::cout << "Music Error: " << e.what() << "\n";
        }
    }

    void SoundManager::streamMusic(const std::string& filename)
    {
        streamMusicFull("Music/"+filename);
    }

    void SoundManager::startRandomTitle()
    {
        Ogre::StringVector filelist;
        if (mMusicFiles.find(mCurrentPlaylist) == mMusicFiles.end())
        {
            Ogre::StringVector groups = Ogre::ResourceGroupManager::getSingleton().getResourceGroups ();
            for (Ogre::StringVector::iterator it = groups.begin(); it != groups.end(); ++it)
            {
                Ogre::StringVectorPtr resourcesInThisGroup = mResourceMgr.findResourceNames(*it,
                                                                                            "Music/"+mCurrentPlaylist+"/*");
                filelist.insert(filelist.end(), resourcesInThisGroup->begin(), resourcesInThisGroup->end());
            }
            mMusicFiles[mCurrentPlaylist] = filelist;
        }
        else
            filelist = mMusicFiles[mCurrentPlaylist];

        if(!filelist.size())
            return;

        int i = OEngine::Misc::Rng::rollDice(filelist.size());

        // Don't play the same music track twice in a row
        if (filelist[i] == mLastPlayedMusic)
        {
            i = (i+1) % filelist.size();
        }

        streamMusicFull(filelist[i]);
    }

    bool SoundManager::isMusicPlaying()
    {
        return mMusic && mMusic->isPlaying();
    }

    void SoundManager::playPlaylist(const std::string &playlist)
    {
        mCurrentPlaylist = playlist;
        startRandomTitle();
    }

    void SoundManager::say(const MWWorld::Ptr &ptr, const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        try
        {
            float basevol = volumeFromType(Play_TypeVoice);
            std::string filePath = "Sound/"+filename;
            const ESM::Position &pos = ptr.getRefData().getPosition();
            const Ogre::Vector3 objpos(pos.pos);

            MWBase::World* world = MWBase::Environment::get().getWorld();
            static const float fAudioMinDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMinDistanceMult")->getFloat();
            static const float fAudioMaxDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMaxDistanceMult")->getFloat();
            static const float fAudioVoiceDefaultMinDistance = world->getStore().get<ESM::GameSetting>().find("fAudioVoiceDefaultMinDistance")->getFloat();
            static const float fAudioVoiceDefaultMaxDistance = world->getStore().get<ESM::GameSetting>().find("fAudioVoiceDefaultMaxDistance")->getFloat();

            float minDistance = fAudioVoiceDefaultMinDistance * fAudioMinDistanceMult;
            float maxDistance = fAudioVoiceDefaultMaxDistance * fAudioMaxDistanceMult;
            minDistance = std::max(minDistance, 1.f);
            maxDistance = std::max(minDistance, maxDistance);

            MWBase::SoundPtr sound = mOutput->playSound3D(filePath, objpos, 1.0f, basevol, 1.0f,
                                                          minDistance, maxDistance, Play_Normal|Play_TypeVoice, 0, true);
            mActiveSounds[sound] = std::make_pair(ptr, std::string("_say_sound"));
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    float SoundManager::getSaySoundLoudness(const MWWorld::Ptr &ptr) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == "_say_sound")
                break;
            ++snditer;
        }
        if (snditer == mActiveSounds.end())
            return 0.f;

        return snditer->first->getCurrentLoudness();
    }

    void SoundManager::say(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        try
        {
            float basevol = volumeFromType(Play_TypeVoice);
            std::string filePath = "Sound/"+filename;

            MWBase::SoundPtr sound = mOutput->playSound(filePath, 1.0f, basevol, 1.0f, Play_Normal|Play_TypeVoice, 0);
            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), std::string("_say_sound"));
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    bool SoundManager::sayDone(const MWWorld::Ptr &ptr) const
    {
        return !isPlaying(ptr, "_say_sound");
    }

    void SoundManager::stopSay(const MWWorld::Ptr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == "_say_sound")
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }


    MWBase::SoundPtr SoundManager::playTrack(const DecoderPtr& decoder, PlayType type)
    {
        MWBase::SoundPtr track;
        if(!mOutput->isInitialized())
            return track;
        try
        {
            track = mOutput->streamSound(decoder, volumeFromType(type), 1.0f, Play_NoEnv|type);
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return track;
    }


    MWBase::SoundPtr SoundManager::playSound(const std::string& soundId, float volume, float pitch, PlayType type, PlayMode mode, float offset)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            float basevol = volumeFromType(type);
            float min, max;
            std::string file = lookup(soundId, volume, min, max);

            sound = mOutput->playSound(file, volume, basevol, pitch, mode|type, offset);
            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
        }
        catch(std::exception&)
        {
            //std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    MWBase::SoundPtr SoundManager::playSound3D(const MWWorld::Ptr &ptr, const std::string& soundId,
                                               float volume, float pitch, PlayType type, PlayMode mode, float offset)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            // Look up the sound in the ESM data
            float basevol = volumeFromType(type);
            float min, max;
            std::string file = lookup(soundId, volume, min, max);
            const ESM::Position &pos = ptr.getRefData().getPosition();
            const Ogre::Vector3 objpos(pos.pos);

            if ((mode & Play_RemoveAtDistance) && mListenerPos.squaredDistance(objpos) > 2000*2000)
            {
                return MWBase::SoundPtr();
            }

            sound = mOutput->playSound3D(file, objpos, volume, basevol, pitch, min, max, mode|type, offset);
            if((mode&Play_NoTrack))
                mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
            else
                mActiveSounds[sound] = std::make_pair(ptr, soundId);
        }
        catch(std::exception&)
        {
            //std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    MWBase::SoundPtr SoundManager::playManualSound3D(const Ogre::Vector3& initialPos, const std::string& soundId,
                                                     float volume, float pitch, PlayType type, PlayMode mode, float offset)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            // Look up the sound in the ESM data
            float basevol = volumeFromType(type);
            float min, max;
            std::string file = lookup(soundId, volume, min, max);

            sound = mOutput->playSound3D(file, initialPos, volume, basevol, pitch, min, max, mode|type, offset);
            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
        }
        catch(std::exception &)
        {
            //std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    void SoundManager::stopSound (MWBase::SoundPtr sound)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->first == sound)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound3D(const MWWorld::Ptr &ptr, const std::string& soundId)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == soundId)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound3D(const MWWorld::Ptr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound(const MWWorld::CellStore *cell)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first != MWWorld::Ptr() &&
               snditer->second.first != MWBase::Environment::get().getWorld()->getPlayerPtr() &&
               snditer->second.first.getCell() == cell)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound(const std::string& soundId)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == MWWorld::Ptr() &&
               snditer->second.second == soundId)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::fadeOutSound3D(const MWWorld::Ptr &ptr,
            const std::string& soundId, float duration)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == soundId)
            {
                snditer->first->setFadeout(duration);
            }
            ++snditer;
        }
    }

    bool SoundManager::getSoundPlaying(const MWWorld::Ptr &ptr, const std::string& soundId) const
    {
        return isPlaying(ptr, soundId);
    }


    void SoundManager::pauseSounds(int types)
    {
        if(mOutput->isInitialized())
        {
            types &= Play_TypeMask;
            mOutput->pauseSounds(types);
            mPausedSoundTypes |= types;
        }
    }

    void SoundManager::resumeSounds(int types)
    {
        if(mOutput->isInitialized())
        {
            types &= types&Play_TypeMask&mPausedSoundTypes;
            mOutput->resumeSounds(types);
            mPausedSoundTypes &= ~types;
        }
    }


    void SoundManager::updateRegionSound(float duration)
    {
        static float sTimeToNextEnvSound = 0.0f;
        static int total = 0;
        static std::string regionName = "";
        static float sTimePassed = 0.0;
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Ptr player = world->getPlayerPtr();
        const ESM::Cell *cell = player.getCell()->getCell();

        sTimePassed += duration;
        if(!cell->isExterior() || sTimePassed < sTimeToNextEnvSound)
            return;

        float a = OEngine::Misc::Rng::rollClosedProbability();
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
        if(regn == NULL)
            return;

        std::vector<ESM::Region::SoundRef>::const_iterator soundIter;
        if(total == 0)
        {
            soundIter = regn->mSoundList.begin();
            while(soundIter != regn->mSoundList.end())
            {
                total += (int)soundIter->mChance;
                ++soundIter;
            }
            if(total == 0)
                return;
        }

        int r = OEngine::Misc::Rng::rollDice(total);
        int pos = 0;

        soundIter = regn->mSoundList.begin();
        while(soundIter != regn->mSoundList.end())
        {
            if(r - pos < soundIter->mChance)
            {
                playSound(soundIter->mSound.toString(), 1.0f, 1.0f);
                break;
            }
            pos += soundIter->mChance;

            ++soundIter;
        }
    }

    void SoundManager::updateSounds(float duration)
    {
        static float timePassed = 0.0;

        timePassed += duration;
        if(timePassed < (1.0f/30.0f))
            return;
        duration = timePassed;
        timePassed = 0.0f;

        // Make sure music is still playing
        if(!isMusicPlaying())
            startRandomTitle();

        Environment env = Env_Normal;
        if (mListenerUnderwater)
        {
            env = Env_Underwater;
            //play underwater sound
            if(!(mUnderwaterSound && mUnderwaterSound->isPlaying()))
                mUnderwaterSound = playSound("Underwater", 1.0f, 1.0f, Play_TypeSfx, Play_LoopNoEnv);
        }
        else if(mUnderwaterSound)
        {
            mUnderwaterSound->stop();
            mUnderwaterSound.reset();
        }

        mOutput->updateListener(
            mListenerPos,
            mListenerDir,
            mListenerUp,
            env
        );

        // Check if any sounds are finished playing, and trash them
        // Lower volume on fading out sounds
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(!snditer->first->isPlaying())
                mActiveSounds.erase(snditer++);
            else
            {
                const MWWorld::Ptr &ptr = snditer->second.first;
                if(!ptr.isEmpty())
                {
                    const ESM::Position &pos = ptr.getRefData().getPosition();
                    const Ogre::Vector3 objpos(pos.pos);
                    snditer->first->setPosition(objpos);

                    if ((snditer->first->mFlags & Play_RemoveAtDistance)
                            && mListenerPos.squaredDistance(Ogre::Vector3(ptr.getRefData().getPosition().pos)) > 2000*2000)
                    {
                        mActiveSounds.erase(snditer++);
                        continue;
                    }
                }
                //update fade out
                if(snditer->first->mFadeOutTime>0)
                {
                    float soundDuration=duration;
                    if(soundDuration>snditer->first->mFadeOutTime)
                        soundDuration=snditer->first->mFadeOutTime;
                    snditer->first->setVolume(snditer->first->mVolume
                                    - soundDuration / snditer->first->mFadeOutTime * snditer->first->mVolume);
                    snditer->first->mFadeOutTime -= soundDuration;
                }
                snditer->first->update();
                ++snditer;
            }
        }
    }

    void SoundManager::update(float duration)
    {
        if(!mOutput->isInitialized())
            return;

        if (MWBase::Environment::get().getStateManager()->getState()!=
            MWBase::StateManager::State_NoGame)
        {
            updateSounds(duration);
            updateRegionSound(duration);
        }
    }


    void SoundManager::processChangedSettings(const Settings::CategorySettingVector& settings)
    {
        mMasterVolume = Settings::Manager::getFloat("master volume", "Sound");
        mMusicVolume = Settings::Manager::getFloat("music volume", "Sound");
        mSFXVolume = Settings::Manager::getFloat("sfx volume", "Sound");
        mFootstepsVolume = Settings::Manager::getFloat("footsteps volume", "Sound");
        mVoiceVolume = Settings::Manager::getFloat("voice volume", "Sound");

        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            snditer->first->mBaseVolume = volumeFromType(snditer->first->getPlayType());
            snditer->first->update();
            ++snditer;
        }
        if(mMusic)
        {
            mMusic->mBaseVolume = volumeFromType(mMusic->getPlayType());
            mMusic->update();
        }
    }

    void SoundManager::setListenerPosDir(const Ogre::Vector3 &pos, const Ogre::Vector3 &dir, const Ogre::Vector3 &up)
    {
        mListenerPos = pos;
        mListenerDir = dir;
        mListenerUp  = up;

        MWWorld::Ptr player =
            MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWWorld::CellStore *cell = player.getCell();

        mListenerUnderwater = ((cell->getCell()->mData.mFlags&ESM::Cell::HasWater) && mListenerPos.z < cell->getWaterLevel());
    }

    void SoundManager::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        for (SoundMap::iterator snditer = mActiveSounds.begin(); snditer != mActiveSounds.end(); ++snditer)
        {
            if (snditer->second.first == old)
                snditer->second.first = updated;
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
        for (SoundMap::iterator iter (mActiveSounds.begin()); iter!=mActiveSounds.end(); ++iter)
            iter->first->stop();

        mActiveSounds.clear();
        stopMusic();
    }
}

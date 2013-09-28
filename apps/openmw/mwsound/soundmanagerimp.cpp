#include "soundmanagerimp.hpp"

#include <iostream>
#include <algorithm>
#include <map>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "sound_output.hpp"
#include "sound_decoder.hpp"
#include "sound.hpp"

#include "openal_output.hpp"
#define SOUND_OUT "OpenAL"
/* Set up the sound manager to use FFMPEG, MPG123+libsndfile, or Audiere for
 * input. The OPENMW_USE_x macros are set in CMakeLists.txt.
*/
#ifdef OPENMW_USE_FFMPEG
#include "ffmpeg_decoder.hpp"
#ifndef SOUND_IN
#define SOUND_IN "FFmpeg"
#endif
#endif

#ifdef OPENMW_USE_AUDIERE
#include "audiere_decoder.hpp"
#ifndef SOUND_IN
#define SOUND_IN "Audiere"
#endif
#endif

#ifdef OPENMW_USE_MPG123
#include "mpgsnd_decoder.hpp"
#ifndef SOUND_IN
#define SOUND_IN "mpg123,sndfile"
#endif
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
                std::cout <<"Failed to open device \""<<devname<<"\", trying default."<< std::endl << "The error given was: " << e.what() << std::endl;
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
        const ESM::Sound *snd = MWBase::Environment::get().getWorld()->getStore().sounds.search(soundId);
        if(snd == NULL)
            throw std::runtime_error(std::string("Failed to lookup sound ")+soundId);

        volume *= pow(10.0, (snd->data.volume/255.0*3348.0 - 3348.0) / 2000.0);

        if(snd->data.minRange == 0 && snd->data.maxRange == 0)
        {
            min = 100.0f;
            max = 2000.0f;
        }
        else
        {
            min = snd->data.minRange * 20.0f;
            max = snd->data.maxRange * 50.0f;
            min = std::max(min, 1.0f);
            max = std::max(min, max);
        }

        return "Sound/"+snd->sound;
    }


    bool SoundManager::isPlaying(MWWorld::Ptr ptr, const std::string &id) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == id)
                return snditer->first->isPlaying();
            snditer++;
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
        try
        {
            float basevol = mMasterVolume * mMusicVolume;
            stopMusic();
            mMusic = mOutput->streamSound(filename, basevol, 1.0f, Play_NoEnv);
            mMusic->mBaseVolume = basevol;
            mMusic->mFlags = Play_NoEnv;
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
        Ogre::StringVectorPtr filelist;
        filelist = mResourceMgr.findResourceNames(Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                  "Music/"+mCurrentPlaylist+"/*");
        if(!filelist->size())
            return;

        int i = rand()%filelist->size();
        streamMusicFull((*filelist)[i]);
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

    void SoundManager::say(MWWorld::Ptr ptr, const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        try
        {
            // The range values are not tested
            float basevol = mMasterVolume * mVoiceVolume;
            std::string filePath = "Sound/"+filename;
            const ESM::Position &pos = ptr.getRefData().getPosition();
            const Ogre::Vector3 objpos(pos.pos[0], pos.pos[1], pos.pos[2]);

            MWBase::SoundPtr sound = mOutput->playSound3D(filePath, objpos, basevol, 1.0f,
                                                  20.0f, 12750.0f, Play_Normal);
            sound->mPos = objpos;
            sound->mBaseVolume = basevol;

            mActiveSounds[sound] = std::make_pair(ptr, std::string("_say_sound"));
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    void SoundManager::say(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        try
        {
            float basevol = mMasterVolume * mVoiceVolume;
            std::string filePath = "Sound/"+filename;

            MWBase::SoundPtr sound = mOutput->playSound(filePath, basevol, 1.0f, Play_Normal);
            sound->mBaseVolume = basevol;

            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), std::string("_say_sound"));
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    bool SoundManager::sayDone(MWWorld::Ptr ptr) const
    {
        return !isPlaying(ptr, "_say_sound");
    }

    void SoundManager::stopSay(MWWorld::Ptr ptr)
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
                snditer++;
        }
    }



    MWBase::SoundPtr SoundManager::playSound(const std::string& soundId, float volume, float pitch, int mode)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            float basevol = mMasterVolume * mSFXVolume;
            float min, max;
            std::string file = lookup(soundId, basevol, min, max);

            sound = mOutput->playSound(file, volume*basevol, pitch, mode);
            sound->mVolume = volume;
            sound->mBaseVolume = basevol;
            sound->mPitch = pitch;
            sound->mMinDistance = min;
            sound->mMaxDistance = max;
            sound->mFlags = mode;

            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    MWBase::SoundPtr SoundManager::playSound3D(MWWorld::Ptr ptr, const std::string& soundId,
                                       float volume, float pitch, int mode)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            // Look up the sound in the ESM data
            float basevol = mMasterVolume * mSFXVolume;
            float min, max;
            std::string file = lookup(soundId, basevol, min, max);
            const ESM::Position &pos = ptr.getRefData().getPosition();;
            const Ogre::Vector3 objpos(pos.pos[0], pos.pos[1], pos.pos[2]);

            sound = mOutput->playSound3D(file, objpos, volume*basevol, pitch, min, max, mode);
            sound->mPos = objpos;
            sound->mVolume = volume;
            sound->mBaseVolume = basevol;
            sound->mPitch = pitch;
            sound->mMinDistance = min;
            sound->mMaxDistance = max;
            sound->mFlags = mode;

            if((mode&Play_NoTrack))
                mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
            else
                mActiveSounds[sound] = std::make_pair(ptr, soundId);
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    void SoundManager::stopSound3D(MWWorld::Ptr ptr, const std::string& soundId)
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
                snditer++;
        }
    }

    void SoundManager::stopSound3D(MWWorld::Ptr ptr)
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
                snditer++;
        }
    }

    void SoundManager::stopSound(const MWWorld::Ptr::CellStore *cell)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first != MWWorld::Ptr() &&
               snditer->second.first.getCell() == cell)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                snditer++;
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
                snditer++;
        }
    }

    bool SoundManager::getSoundPlaying(MWWorld::Ptr ptr, const std::string& soundId) const
    {
        return isPlaying(ptr, soundId);
    }

    void SoundManager::updateObject(MWWorld::Ptr ptr)
    {
        const ESM::Position &pos = ptr.getRefData().getPosition();;
        const Ogre::Vector3 objpos(pos.pos[0], pos.pos[1], pos.pos[2]);
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr)
                snditer->first->setPosition(objpos);
            snditer++;
        }
    }

    void SoundManager::updateRegionSound(float duration)
    {
        MWWorld::Ptr::CellStore *current = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell();
        static int total = 0;
        static std::string regionName = "";
        static float timePassed = 0.0;

        //If the region has changed
        timePassed += duration;
        if((current->cell->data.flags & current->cell->Interior) || timePassed < 10)
            return;
        timePassed = 0;

        if(regionName != current->cell->region)
        {
            regionName = current->cell->region;
            total = 0;
        }

        const ESM::Region *regn = MWBase::Environment::get().getWorld()->getStore().regions.search(regionName);
        if (regn == NULL)
            return;

        std::vector<ESM::Region::SoundRef>::const_iterator soundIter;
        if(total == 0)
        {
            soundIter = regn->soundList.begin();
            while(soundIter != regn->soundList.end())
            {
                total += (int)soundIter->chance;
                soundIter++;
            }
            if(total == 0)
                return;
        }

        int r = (int)(rand()/((double)RAND_MAX+1) * total);
        int pos = 0;

        soundIter = regn->soundList.begin();
        while(soundIter != regn->soundList.end())
        {
            const std::string go = soundIter->sound.toString();
            int chance = (int) soundIter->chance;
            //std::cout << "Sound: " << go.name <<" Chance:" <<  chance << "\n";
            soundIter++;
            if(r - pos < chance)
            {
                //play sound
                std::cout << "Sound: " << go <<" Chance:" <<  chance << "\n";
                playSound(go, 1.0f, 1.0f);
                break;
            }
            pos += chance;
        }
    }

    void SoundManager::updateSounds(float duration)
    {
        static float timePassed = 0.0;

        timePassed += duration;
        if(timePassed < (1.0f/30.0f))
            return;
        timePassed = 0.0f;

        // Make sure music is still playing
        if(!isMusicPlaying())
            startRandomTitle();

        MWWorld::Ptr player =
            MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        const ESM::Cell *cell = player.getCell()->cell;

        Environment env = Env_Normal;
        if((cell->data.flags&cell->HasWater) && mListenerPos.z < cell->water)
            env = Env_Underwater;

        mOutput->updateListener(
            mListenerPos,
            mListenerDir,
            Ogre::Vector3::UNIT_Z,
            env
        );

        // Check if any sounds are finished playing, and trash them
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(!snditer->first->isPlaying())
                mActiveSounds.erase(snditer++);
            else
            {
                snditer->first->update();
                snditer++;
            }
        }
    }

    void SoundManager::update(float duration)
    {
        if(!mOutput->isInitialized())
            return;
        updateSounds(duration);
        updateRegionSound(duration);
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
            if(snditer->second.second != "_say_sound")
            {
                float basevol = mMasterVolume * mSFXVolume;
                float min, max;
                lookup(snditer->second.second, basevol, min, max);
                snditer->first->mBaseVolume = basevol;
            }
            else
            {
                float basevol = mMasterVolume * mVoiceVolume;
                snditer->first->mBaseVolume = basevol;
            }
            snditer->first->update();
            snditer++;
        }
        if(mMusic)
        {
            mMusic->mBaseVolume = mMasterVolume * mMusicVolume;
            mMusic->update();
        }
    }

    void SoundManager::setListenerPosDir(const Ogre::Vector3 &pos, const Ogre::Vector3 &dir)
    {
        mListenerPos = pos;
        mListenerDir = dir;
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
        }
        return "(unknown sample type)";
    }

    const char *getChannelConfigName(ChannelConfig config)
    {
        switch(config)
        {
            case ChannelConfig_Mono:   return "Mono";
            case ChannelConfig_Stereo: return "Stereo";
        }
        return "(unknown channel config)";
    }

    size_t framesToBytes(size_t frames, ChannelConfig config, SampleType type)
    {
        switch(config)
        {
            case ChannelConfig_Mono:   frames *= 1; break;
            case ChannelConfig_Stereo: frames *= 2; break;
        }
        switch(type)
        {
            case SampleType_UInt8: frames *= 1; break;
            case SampleType_Int16: frames *= 2; break;
        }
        return frames;
    }

    size_t bytesToFrames(size_t bytes, ChannelConfig config, SampleType type)
    {
        return bytes / framesToBytes(1, config, type);
    }
}

#include "soundmanager.hpp"

#include <iostream>
#include <algorithm>
#include <map>

#include <OgreRoot.h>

#include <components/esm_store/store.hpp>

#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/player.hpp"

#include "sound_output.hpp"
#include "sound_decoder.hpp"
#include "sound.hpp"

#include "openal_output.hpp"
#define SOUND_OUT "OpenAL"
/* Set up the sound manager to use FFMPEG or MPG123+libsndfile for input. The
 * OPENMW_USE_x macros are set in CMakeLists.txt.
*/
#ifdef OPENMW_USE_FFMPEG
#include "ffmpeg_decoder.hpp"
#ifndef SOUND_IN
#define SOUND_IN "FFmpeg"
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
    SoundManager::SoundManager(bool useSound, MWWorld::Environment& environment)
        : mResourceMgr(Ogre::ResourceGroupManager::getSingleton())
        , mEnvironment(environment)
    {
        if(!useSound)
            return;

        std::cout << "Sound output: " << SOUND_OUT << std::endl;
        std::cout << "Sound decoder: " << SOUND_IN << std::endl;

        try
        {
            mOutput.reset(new DEFAULT_OUTPUT(*this));

            std::vector<std::string> names = mOutput->enumerate();
            std::cout <<"Enumerated output devices:"<< std::endl;
            for(size_t i = 0;i < names.size();i++)
                std::cout <<"  "<<names[i]<< std::endl;

            mOutput->init();
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound init failed: "<<e.what()<< std::endl;
            mOutput.reset();
            return;
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
        const ESM::Sound *snd = mEnvironment.mWorld->getStore().sounds.search(soundId);
        if(snd == NULL)
            throw std::runtime_error(std::string("Failed to lookup sound ")+soundId);

        if(snd->data.volume == 0)
            volume = 0.0f;
        else
            volume *= pow(10.0, (snd->data.volume/255.0f*3348.0 - 3348.0) / 2000.0);

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

        return std::string("Sound/")+snd->sound;
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
        std::cout <<"Playing "<<filename<< std::endl;
        try
        {
            if(mMusic)
                mMusic->stop();
            mMusic = mOutput->streamSound(filename, 0.4f, 1.0f);
            mMusic->mBaseVolume = 0.4f;
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
        try
        {
            // The range values are not tested
            float basevol = 1.0f; /* TODO: volume settings */
            std::string filePath = std::string("Sound/")+filename;
            const ESM::Position &pos = ptr.getCellRef().pos;

            SoundPtr sound = mOutput->playSound3D(filePath, pos.pos, basevol, 1.0f, 20.0f, 12750.0f, false);
            sound->mBaseVolume = basevol;

            mActiveSounds[sound] = std::make_pair(ptr, std::string("_say_sound"));
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


    void SoundManager::playSound(const std::string& soundId, float volume, float pitch, bool loop)
    {
        try
        {
            float basevol = 1.0f; /* TODO: volume settings */
            float min, max;
            std::string file = lookup(soundId, basevol, min, max);

            SoundPtr sound = mOutput->playSound(file, volume*basevol, pitch, loop);
            sound->mVolume = volume;
            sound->mBaseVolume = basevol;
            sound->mMinDistance = min;
            sound->mMaxDistance = max;

            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    void SoundManager::playSound3D(MWWorld::Ptr ptr, const std::string& soundId,
                                   float volume, float pitch, bool loop, bool untracked)
    {
        try
        {
            // Look up the sound in the ESM data
            float basevol = 1.0f; /* TODO: volume settings */
            float min, max;
            std::string file = lookup(soundId, basevol, min, max);
            const ESM::Position &pos = ptr.getCellRef().pos;

            SoundPtr sound = mOutput->playSound3D(file, pos.pos, volume*basevol, pitch, min, max, loop);
            sound->mVolume = volume;
            sound->mBaseVolume = basevol;
            sound->mMinDistance = min;
            sound->mMaxDistance = max;

            mActiveSounds[sound] = (!untracked ? std::make_pair(ptr, soundId) :
                                    std::make_pair(MWWorld::Ptr(), soundId));
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
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
        const ESM::Position &pos = ptr.getCellRef().pos;
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr)
                snditer->first->update(pos.pos);
            snditer++;
        }
    }

    void SoundManager::updateRegionSound(float duration)
    {
        MWWorld::Ptr::CellStore *current = mEnvironment.mWorld->getPlayer().getPlayer().getCell();
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

        const ESM::Region *regn = mEnvironment.mWorld->getStore().regions.find(regionName);
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

        Ogre::Camera *cam = mEnvironment.mWorld->getPlayer().getRenderer()->getCamera();
        Ogre::Vector3 nPos, nDir, nUp;
        nPos = cam->getRealPosition();
        nDir = cam->getRealDirection();
        nUp  = cam->getRealUp();

        // The output handler is expecting vectors oriented like the game
        // (that is, -Z goes down, +Y goes forward), but that's not what we
        // get from Ogre's camera, so we have to convert.
        float pos[3] = { nPos[0], -nPos[2], nPos[1] };
        float at[3] = { nDir[0], -nDir[2], nDir[1] };
        float up[3] = { nUp[0], -nUp[2], nUp[1] };
        mOutput->updateListener(pos, at, up);

        // Check if any sounds are finished playing, and trash them
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(!snditer->first->isPlaying())
                mActiveSounds.erase(snditer++);
            else
                snditer++;
        }
    }

    void SoundManager::update(float duration)
    {
        updateSounds(duration);
        updateRegionSound(duration);
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

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
#define SOUND_IN "FFmpeg"
#endif

#ifdef OPENMW_USE_MPG123
#include "mpgsnd_decoder.hpp"
#define SOUND_IN "mpg123,sndfile"
#endif


namespace MWSound
{
    SoundManager::SoundManager(Ogre::Root *root, Ogre::Camera *camera,
        const Files::PathContainer& dataDirs,
        bool useSound, bool fsstrict, MWWorld::Environment& environment)
        : mFSStrict(fsstrict)
        , mEnvironment(environment)
        , mCurrentPlaylist(NULL)
    {
        if(!useSound)
            return;

        std::cout << "Sound output: " << SOUND_OUT << std::endl;
        std::cout << "Sound decoder: " << SOUND_IN << std::endl;

        try
        {
            mOutput.reset(new DEFAULT_OUTPUT(*this));
            mOutput->init();
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound init failed: "<<e.what()<< std::endl;
            mOutput.reset();
            return;
        }

        // The music library will accept these filetypes
        // If none is given then it will accept all filetypes
        std::vector<std::string> acceptableExtensions;
        acceptableExtensions.push_back(".mp3");
        acceptableExtensions.push_back(".wav");
        acceptableExtensions.push_back(".ogg");
        acceptableExtensions.push_back(".flac");

        // Makes a list of all sound files, searches in reverse for priority reasons
        for(Files::PathContainer::const_reverse_iterator it = dataDirs.rbegin(); it != dataDirs.rend(); ++it)
            Files::FileLister(*it / std::string("Sound"), mSoundFiles, true);

        // Makes a FileLibrary of all music files, searches in reverse for priority reasons
        for(Files::PathContainer::const_reverse_iterator it = dataDirs.rbegin(); it != dataDirs.rend(); ++it)
            mMusicLibrary.add(*it / std::string("Music"), true, mFSStrict, acceptableExtensions);

        std::string anything = "anything";      // anything is better that a segfault
        mCurrentPlaylist = mMusicLibrary.section(anything, mFSStrict); // now points to an empty path
    }

    SoundManager::~SoundManager()
    {
        mLooseSounds.clear();
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
        if(snd == NULL) return "";

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

        return Files::FileListLocator(mSoundFiles, snd->sound, mFSStrict, false);
    }

    // Add a sound to the list and play it
    void SoundManager::play3d(const std::string &file,
             MWWorld::Ptr ptr,
             const std::string &id,
             float volume, float pitch,
             float min, float max,
             bool loop, bool untracked)
    {
        try
        {
            const ESM::Position &pos = ptr.getCellRef().pos;
            SoundPtr sound(mOutput->playSound3D(file, pos.pos, volume, pitch, min, max, loop));

            if(untracked) mLooseSounds[id] = sound;
            else mActiveSounds[ptr][id] = sound;
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    bool SoundManager::isPlaying(MWWorld::Ptr ptr, const std::string &id) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.find(ptr);
        if(snditer == mActiveSounds.end())
            return false;

        IDMap::const_iterator iditer = snditer->second.find(id);
        if(iditer == snditer->second.end())
            return false;

        return true;
    }


    void SoundManager::stopMusic()
    {
        if(mMusic)
            mMusic->stop();
        setPlaylist();
    }

    void SoundManager::streamMusicFull(const std::string& filename)
    {
        if(mMusic)
            mMusic->stop();
        mMusic.reset(mOutput->streamSound(filename, 0.4f, 1.0f));
    }

    void SoundManager::streamMusic(const std::string& filename)
    {
        std::string filePath = mMusicLibrary.locate(filename, mFSStrict, true).string();
        if(!filePath.empty())
        {
            try
            {
                streamMusicFull(filePath);
            }
            catch(std::exception &e)
            {
                std::cout << "Music Error: " << e.what() << "\n";
            }
        }
    }

    void SoundManager::startRandomTitle()
    {
        if(mCurrentPlaylist && !mCurrentPlaylist->empty())
        {
            Files::PathContainer::const_iterator fileIter = mCurrentPlaylist->begin();
            srand( time(NULL) );
            int r = rand() % mCurrentPlaylist->size() + 1;        //old random code

            std::advance(fileIter, r - 1);
            std::string music = fileIter->string();
            //std::cout << "Playing " << music << "\n";

            try
            {
                streamMusicFull(music);
            }
            catch (std::exception &e)
            {
                std::cout << "Music Error: " << e.what() << "\n";
            }
        }
    }

    bool SoundManager::isMusicPlaying()
    {
        return mMusic && mMusic->isPlaying();
    }

    bool SoundManager::setPlaylist(std::string playlist)
    {
        const Files::PathContainer* previousPlaylist;
        previousPlaylist = mCurrentPlaylist;
        if (playlist == "")
        {
            mCurrentPlaylist = mMusicLibrary.section(playlist, mFSStrict);
        }
        else if(mMusicLibrary.containsSection(playlist, mFSStrict))
        {
            mCurrentPlaylist = mMusicLibrary.section(playlist, mFSStrict);
        }
        else
        {
            std::cout << "Warning: playlist named " << playlist << " does not exist.\n";
        }
        return previousPlaylist == mCurrentPlaylist;
    }

    void SoundManager::playPlaylist(std::string playlist)
    {
        if (playlist == "")
        {
            if(!isMusicPlaying())
            {
                startRandomTitle();
            }
            return;
        }

        if(!setPlaylist(playlist))
        {
            startRandomTitle();
        }
        else if (!isMusicPlaying())
        {
            startRandomTitle();
        }
    }

    void SoundManager::say(MWWorld::Ptr ptr, const std::string& filename)
    {
        // The range values are not tested
        std::string filePath = Files::FileListLocator(mSoundFiles, filename, mFSStrict, true);
        if(!filePath.empty())
            play3d(filePath, ptr, "_say_sound", 1, 1, 100, 20000, false);
    }

    bool SoundManager::sayDone(MWWorld::Ptr ptr) const
    {
        return !isPlaying(ptr, "_say_sound");
    }


    void SoundManager::playSound(const std::string& soundId, float volume, float pitch, bool loop)
    {
        float min, max;
        std::string file = lookup(soundId, volume, min, max);
        if(!file.empty())
        {
            try
            {
                Sound *sound;
                sound = mOutput->playSound(file, volume, pitch, loop);
                mLooseSounds[soundId] = SoundPtr(sound);
            }
            catch(std::exception &e)
            {
                std::cout <<"Sound play error: "<<e.what()<< std::endl;
            }
        }
        else
            std::cout << "Sound file " << soundId << " not found, skipping.\n";
    }

    void SoundManager::playSound3D(MWWorld::Ptr ptr, const std::string& soundId,
                                   float volume, float pitch, bool loop, bool untracked)
    {
        // Look up the sound in the ESM data
        float min, max;
        std::string file = lookup(soundId, volume, min, max);
        if(!file.empty())
            play3d(file, ptr, soundId, volume, pitch, min, max, loop, untracked);
        else
            std::cout << "Sound file " << soundId << " not found, skipping.\n";
    }

    void SoundManager::stopSound3D(MWWorld::Ptr ptr, const std::string& soundId)
    {
        // Stop a sound and remove it from the list. If soundId="" then
        // stop all its sounds.
        SoundMap::iterator snditer = mActiveSounds.find(ptr);
        if(snditer == mActiveSounds.end())
            return;

        if(!soundId.empty())
        {
            IDMap::iterator iditer = snditer->second.find(soundId);
            if(iditer != snditer->second.end())
            {
                iditer->second->stop();
                snditer->second.erase(iditer);
                if(snditer->second.empty())
                    mActiveSounds.erase(snditer);
            }
        }
        else
        {
            IDMap::iterator iditer = snditer->second.begin();
            while(iditer != snditer->second.end())
            {
                iditer->second->stop();
                iditer++;
            }
            mActiveSounds.erase(snditer);
        }
    }

    void SoundManager::stopSound(MWWorld::Ptr::CellStore *cell)
    {
        // Remove all references to objects belonging to a given cell
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->first.getCell() == cell)
            {
                IDMap::iterator iditer = snditer->second.begin();
                while(iditer != snditer->second.end())
                {
                    iditer->second->stop();
                    iditer++;
                }
                mActiveSounds.erase(snditer++);
            }
            else
                snditer++;
        }
    }

    void SoundManager::stopSound(const std::string& soundId)
    {
        IDMap::iterator iditer = mLooseSounds.find(soundId);
        if(iditer != mLooseSounds.end())
        {
            iditer->second->stop();
            mLooseSounds.erase(iditer);
        }
    }

    bool SoundManager::getSoundPlaying(MWWorld::Ptr ptr, const std::string& soundId) const
    {
        return isPlaying(ptr, soundId);
    }

    void SoundManager::updateObject(MWWorld::Ptr ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.find(ptr);
        if(snditer == mActiveSounds.end())
            return;

        IDMap::iterator iditer = snditer->second.begin();
        while(iditer != snditer->second.end())
        {
            const ESM::Position &pos = ptr.getCellRef().pos;
            iditer->second->update(pos.pos);
            iditer++;
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

        ESM::Region test = (ESM::Region) *(mEnvironment.mWorld->getStore().regions.find(current->cell->region));

        timePassed = 0;
        if(regionName != current->cell->region)
        {
            regionName = current->cell->region;
            total = 0;
        }

        if(test.soundList.size() == 0)
            return;

        std::vector<ESM::Region::SoundRef>::iterator soundIter;
        if(total == 0)
        {
            soundIter = test.soundList.begin();
            while(soundIter != test.soundList.end())
            {
                int chance = (int) soundIter->chance;
                //ESM::NAME32 go = soundIter->sound;
                //std::cout << "Sound: " << go.name <<" Chance:" <<  chance << "\n";
                soundIter++;
                total += chance;
            }
        }

        int r = rand() % total;        //old random code
        int pos = 0;

        soundIter = test.soundList.begin();
        while(soundIter != test.soundList.end())
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

    void SoundManager::update(float duration)
    {
        static float timePassed = 0.0;

        timePassed += duration;
        if(timePassed > (1.0f/30.0f))
        {
            timePassed = 0.0f;

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
                IDMap::iterator iditer = snditer->second.begin();
                while(iditer != snditer->second.end())
                {
                    if(!iditer->second->isPlaying())
                        snditer->second.erase(iditer++);
                    else
                        iditer++;
                }
                if(snditer->second.empty())
                    mActiveSounds.erase(snditer++);
                else
                    snditer++;
            }

            IDMap::iterator iditer = mLooseSounds.begin();
            while(iditer != mLooseSounds.end())
            {
                if(!iditer->second->isPlaying())
                    mLooseSounds.erase(iditer++);
                else
                    iditer++;
            }
        }

        updateRegionSound(duration);
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

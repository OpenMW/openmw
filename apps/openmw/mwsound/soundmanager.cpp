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
/* Set up the sound manager to use Audiere, FFMPEG or
   MPG123/libsndfile for input. The OPENMW_USE_x macros are set in
   CMakeLists.txt.
*/
#ifdef OPENMW_USE_FFMPEG
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

        Output.reset(new DEFAULT_OUTPUT(*this));
        if(!Output->Initialize())
        {
            Output.reset();
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
        if(mMusic)
            mMusic->Stop();
        mMusic.reset();
        Output.reset();
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
    void SoundManager::add(const std::string &file,
             MWWorld::Ptr ptr,
             const std::string &id,
             float volume, float pitch,
             float min, float max,
             bool loop, bool untracked)
    {
        try
        {
            Sound *sound;
            std::auto_ptr<Sound_Decoder> decoder(new DEFAULT_DECODER);
            sound = Output->PlaySound3D(file, decoder, ptr, volume, pitch, min, max, loop);
            delete sound;
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound play error: "<<e.what()<< std::endl;
        }
    }

    // Stop a sound and remove it from the list. If id="" then
    // remove the entire object and stop all its sounds.
    void SoundManager::remove(MWWorld::Ptr ptr, const std::string &id)
    {
    }

    bool SoundManager::isPlaying(MWWorld::Ptr ptr, const std::string &id) const
    {
        // HACK: Return true to prevent the engine from trying to keep playing
        // sounds and tanking the framerate.
        return true;
    }

    // Remove all references to objects belonging to a given cell
    void SoundManager::removeCell(const MWWorld::Ptr::CellStore *cell)
    {
    }

    void SoundManager::updatePositions(MWWorld::Ptr ptr)
    {
    }

    void SoundManager::stopMusic()
    {
        if(mMusic)
            mMusic->Stop();
        setPlaylist();
    }


    void SoundManager::streamMusicFull(const std::string& filename)
    {
        if(mMusic)
            mMusic->Stop();
        std::auto_ptr<Sound_Decoder> decoder(new DEFAULT_DECODER);
        mMusic.reset(Output->StreamSound(filename, decoder));
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
            add(filePath, ptr, "_say_sound", 1, 1, 100, 20000, false);
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
                std::auto_ptr<Sound_Decoder> decoder(new DEFAULT_DECODER);
                sound = Output->PlaySound(file, decoder, volume, pitch, loop);
                delete sound;
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
            add(file, ptr, soundId, volume, pitch, min, max, false);
        else
            std::cout << "Sound file " << soundId << " not found, skipping.\n";
    }

    void SoundManager::stopSound3D(MWWorld::Ptr ptr, const std::string& soundId)
    {
        remove(ptr, soundId);
    }

    void SoundManager::stopSound(MWWorld::Ptr::CellStore *cell)
    {
        removeCell(cell);
    }

    void SoundManager::stopSound(const std::string& soundId)
    {
    }

    bool SoundManager::getSoundPlaying(MWWorld::Ptr ptr, const std::string& soundId) const
    {
        // Mark all sounds as playing, otherwise the scripts will just
        // keep trying to play them every frame.

        return isPlaying(ptr, soundId);
    }

    void SoundManager::updateObject(MWWorld::Ptr ptr)
    {
        updatePositions(ptr);
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
                playSound(go, 20.0, 1.0);
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

            Output->UpdateListener(&nPos[0], &nDir[0], &nUp[0]);
        }

        updateRegionSound(duration);
    }
}

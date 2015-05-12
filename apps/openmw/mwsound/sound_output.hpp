#ifndef GAME_SOUND_SOUND_OUTPUT_H
#define GAME_SOUND_SOUND_OUTPUT_H

#include <string>
#include <memory>

#include "soundmanagerimp.hpp"

#include "../mwworld/ptr.hpp"

namespace MWSound
{
    class SoundManager;
    struct Sound_Decoder;
    class Sound;

    class Sound_Output
    {
        SoundManager &mManager;

        virtual std::vector<std::string> enumerate() = 0;
        virtual void init(const std::string &devname="") = 0;
        virtual void deinit() = 0;

        /// @param offset Value from [0,1] meaning from which fraction the sound the playback starts.
        virtual MWBase::SoundPtr playSound(const std::string &fname, float vol, float basevol, float pitch, int flags, float offset) = 0;
        /// @param offset Value from [0,1] meaning from which fraction the sound the playback starts.
        virtual MWBase::SoundPtr playSound3D(const std::string &fname, const osg::Vec3f &pos,
                                             float vol, float basevol, float pitch, float min, float max, int flags, float offset, bool extractLoudness=false) = 0;
        virtual MWBase::SoundPtr streamSound(DecoderPtr decoder, float volume, float pitch, int flags) = 0;

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env) = 0;

        virtual void pauseSounds(int types) = 0;
        virtual void resumeSounds(int types) = 0;

        Sound_Output& operator=(const Sound_Output &rhs);
        Sound_Output(const Sound_Output &rhs);

    protected:
        bool mInitialized;
        osg::Vec3f mPos;

        Sound_Output(SoundManager &mgr)
          : mManager(mgr)
          , mInitialized(false)
          , mPos(0.0f, 0.0f, 0.0f)
        { }
    public:
        virtual ~Sound_Output() { }

        bool isInitialized() const { return mInitialized; }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif

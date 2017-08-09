#ifndef GAME_SOUND_SOUND_OUTPUT_H
#define GAME_SOUND_SOUND_OUTPUT_H

#include <string>
#include <memory>
#include <vector>

#include "soundmanagerimp.hpp"

namespace MWSound
{
    class SoundManager;
    struct Sound_Decoder;
    class Sound;

    // An opaque handle for the implementation's sound buffers.
    typedef void *Sound_Handle;
    // An opaque handle for the implementation's sound instances.
    typedef void *Sound_Instance;

    class Sound_Output
    {
        SoundManager &mManager;

        virtual std::vector<std::string> enumerate() = 0;
        virtual void init(const std::string &devname=std::string()) = 0;
        virtual void deinit() = 0;

        virtual std::vector<std::string> enumerateHrtf() = 0;
        virtual void enableHrtf(const std::string &hrtfname, bool auto_enable) = 0;
        virtual void disableHrtf() = 0;

        virtual Sound_Handle loadSound(const std::string &fname) = 0;
        virtual void unloadSound(Sound_Handle data) = 0;
        virtual size_t getSoundDataSize(Sound_Handle data) const = 0;

        virtual void playSound(MWBase::SoundPtr sound, Sound_Handle data, float offset) = 0;
        virtual void playSound3D(MWBase::SoundPtr sound, Sound_Handle data, float offset) = 0;
        virtual void finishSound(MWBase::SoundPtr sound) = 0;
        virtual bool isSoundPlaying(MWBase::SoundPtr sound) = 0;
        virtual void updateSound(MWBase::SoundPtr sound) = 0;

        virtual void streamSound(DecoderPtr decoder, MWBase::SoundStreamPtr sound) = 0;
        virtual void streamSound3D(DecoderPtr decoder, MWBase::SoundStreamPtr sound, bool getLoudnessData) = 0;
        virtual void finishStream(MWBase::SoundStreamPtr sound) = 0;
        virtual double getStreamDelay(MWBase::SoundStreamPtr sound) = 0;
        virtual double getStreamOffset(MWBase::SoundStreamPtr sound) = 0;
        virtual float getStreamLoudness(MWBase::SoundStreamPtr sound) = 0;
        virtual bool isStreamPlaying(MWBase::SoundStreamPtr sound) = 0;
        virtual void updateStream(MWBase::SoundStreamPtr sound) = 0;

        virtual void startUpdate() = 0;
        virtual void finishUpdate() = 0;

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env) = 0;

        virtual void pauseSounds(int types) = 0;
        virtual void resumeSounds(int types) = 0;

        Sound_Output& operator=(const Sound_Output &rhs);
        Sound_Output(const Sound_Output &rhs);

    protected:
        bool mInitialized;

        Sound_Output(SoundManager &mgr)
          : mManager(mgr), mInitialized(false)
        { }
    public:
        virtual ~Sound_Output() { }

        bool isInitialized() const { return mInitialized; }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif

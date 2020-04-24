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
    class Stream;

    // An opaque handle for the implementation's sound buffers.
    typedef void *Sound_Handle;
    // An opaque handle for the implementation's sound instances.
    typedef void *Sound_Instance;

    enum class HrtfMode {
        Disable,
        Enable,
        Auto
    };

    class Sound_Output
    {
        SoundManager &mManager;

        virtual std::vector<std::string> enumerate() = 0;
        virtual bool init(const std::string &devname, const std::string &hrtfname, HrtfMode hrtfmode) = 0;
        virtual void deinit() = 0;

        virtual std::vector<std::string> enumerateHrtf() = 0;
        virtual void setHrtf(const std::string &hrtfname, HrtfMode hrtfmode) = 0;

        virtual std::pair<Sound_Handle,size_t> loadSound(const std::string &fname) = 0;
        virtual size_t unloadSound(Sound_Handle data) = 0;

        virtual bool playSound(Sound *sound, Sound_Handle data, float offset) = 0;
        virtual bool playSound3D(Sound *sound, Sound_Handle data, float offset) = 0;
        virtual void finishSound(Sound *sound) = 0;
        virtual bool isSoundPlaying(Sound *sound) = 0;
        virtual void updateSound(Sound *sound) = 0;

        virtual bool streamSound(DecoderPtr decoder, Stream *sound, bool getLoudnessData=false) = 0;
        virtual bool streamSound3D(DecoderPtr decoder, Stream *sound, bool getLoudnessData) = 0;
        virtual void finishStream(Stream *sound) = 0;
        virtual double getStreamDelay(Stream *sound) = 0;
        virtual double getStreamOffset(Stream *sound) = 0;
        virtual float getStreamLoudness(Stream *sound) = 0;
        virtual bool isStreamPlaying(Stream *sound) = 0;
        virtual void updateStream(Stream *sound) = 0;

        virtual void startUpdate() = 0;
        virtual void finishUpdate() = 0;

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env) = 0;

        virtual void pauseSounds(int types) = 0;
        virtual void resumeSounds(int types) = 0;

        virtual void pauseActiveDevice() = 0;
        virtual void resumeActiveDevice() = 0;

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

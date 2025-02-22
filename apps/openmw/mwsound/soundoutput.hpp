#ifndef GAME_SOUND_SOUNDOUTPUT_H
#define GAME_SOUND_SOUNDOUTPUT_H

#include <memory>
#include <string>
#include <vector>

#include <components/settings/hrtfmode.hpp>
#include <components/vfs/pathutil.hpp>

#include "../mwbase/soundmanager.hpp"

namespace MWSound
{
    class SoundManager;
    struct SoundDecoder;
    class Sound;
    class Stream;

    // An opaque handle for the implementation's sound buffers.
    typedef void* Sound_Handle;
    // An opaque handle for the implementation's sound instances.
    typedef void* Sound_Instance;

    enum Environment
    {
        Env_Normal,
        Env_Underwater
    };

    using HrtfMode = Settings::HrtfMode;

    class SoundOutput
    {
        SoundManager& mManager;

        virtual std::vector<std::string> enumerate() = 0;
        virtual bool init(const std::string& devname, const std::string& hrtfname, HrtfMode hrtfmode) = 0;
        virtual void deinit() = 0;

        virtual std::vector<std::string> enumerateHrtf() = 0;

        virtual std::pair<Sound_Handle, size_t> loadSound(VFS::Path::NormalizedView fname) = 0;
        virtual size_t unloadSound(Sound_Handle data) = 0;

        virtual bool playSound(Sound* sound, Sound_Handle data, float offset) = 0;
        virtual bool playSound3D(Sound* sound, Sound_Handle data, float offset) = 0;
        virtual void finishSound(Sound* sound) = 0;
        virtual bool isSoundPlaying(Sound* sound) = 0;
        virtual void updateSound(Sound* sound) = 0;

        virtual bool streamSound(DecoderPtr decoder, Stream* sound, bool getLoudnessData = false) = 0;
        virtual bool streamSound3D(DecoderPtr decoder, Stream* sound, bool getLoudnessData) = 0;
        virtual void finishStream(Stream* sound) = 0;
        virtual double getStreamDelay(Stream* sound) = 0;
        virtual double getStreamOffset(Stream* sound) = 0;
        virtual float getStreamLoudness(Stream* sound) = 0;
        virtual bool isStreamPlaying(Stream* sound) = 0;
        virtual void updateStream(Stream* sound) = 0;

        virtual void startUpdate() = 0;
        virtual void finishUpdate() = 0;

        virtual void updateListener(
            const osg::Vec3f& pos, const osg::Vec3f& atdir, const osg::Vec3f& updir, Environment env)
            = 0;

        virtual void pauseSounds(int types) = 0;
        virtual void resumeSounds(int types) = 0;

        virtual void pauseActiveDevice() = 0;
        virtual void resumeActiveDevice() = 0;

        SoundOutput& operator=(const SoundOutput& rhs);
        SoundOutput(const SoundOutput& rhs);

    protected:
        bool mInitialized;

        SoundOutput(SoundManager& mgr)
            : mManager(mgr)
            , mInitialized(false)
        {
        }

    public:
        virtual ~SoundOutput() {}

        bool isInitialized() const { return mInitialized; }

        friend class OpenALOutput;
        friend class SoundManager;
        friend class SoundBufferPool;
    };
}

#endif

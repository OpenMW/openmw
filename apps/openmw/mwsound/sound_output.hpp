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
    class Sound_Loudness;

    // An opaque handle for the implementation's sound buffers.
    typedef void *Sound_Handle;

    class Sound_Output
    {
        SoundManager &mManager;

        virtual std::vector<std::string> enumerate() = 0;
        virtual void init(const std::string &devname="") = 0;
        virtual void deinit() = 0;

        virtual Sound_Handle loadSound(const std::string &fname) = 0;
        virtual void unloadSound(Sound_Handle data) = 0;
        virtual size_t getSoundDataSize(Sound_Handle data) const = 0;

        /// @param offset Number of seconds into the sound to start playback.
        virtual MWBase::SoundPtr playSound(Sound_Handle data, float vol, float basevol, float pitch, int flags, float offset) = 0;
        /// @param offset Number of seconds into the sound to start playback.
        virtual MWBase::SoundPtr playSound3D(Sound_Handle data, const osg::Vec3f &pos,
                                             float vol, float basevol, float pitch, float min, float max, int flags, float offset) = 0;
        virtual void stopSound(MWBase::SoundPtr sound) = 0;
        virtual bool isSoundPlaying(MWBase::SoundPtr sound) = 0;
        virtual void updateSound(MWBase::SoundPtr sound) = 0;

        virtual MWBase::SoundStreamPtr streamSound(DecoderPtr decoder, float basevol, float pitch, int flags) = 0;
        virtual MWBase::SoundStreamPtr streamSound3D(DecoderPtr decoder, const osg::Vec3f &pos,
                                                     float vol, float basevol, float pitch, float min, float max, int flags) = 0;
        virtual void stopStream(MWBase::SoundStreamPtr sound) = 0;
        virtual double getStreamDelay(MWBase::SoundStreamPtr sound) = 0;
        virtual double getStreamOffset(MWBase::SoundStreamPtr sound) = 0;
        virtual bool isStreamPlaying(MWBase::SoundStreamPtr sound) = 0;
        virtual void updateStream(MWBase::SoundStreamPtr sound) = 0;

        virtual void startUpdate() = 0;
        virtual void finishUpdate() = 0;

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env) = 0;

        virtual void pauseSounds(int types) = 0;
        virtual void resumeSounds(int types) = 0;

        // HACK: The sound output implementation really shouldn't be handling
        // asynchronous loudness data loading, but it's currently where we have
        // a background processing thread.
        virtual void loadLoudnessAsync(DecoderPtr decoder, Sound_Loudness *loudness) = 0;

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

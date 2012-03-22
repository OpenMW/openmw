#ifndef GAME_SOUND_SOUND_OUTPUT_H
#define GAME_SOUND_SOUND_OUTPUT_H

#include <string>
#include <memory>

#include "../mwworld/ptr.hpp"

namespace MWSound
{
    class SoundManager;
    class Sound_Decoder;
    class Sound;

    class Sound_Output
    {
        SoundManager &mManager;

        virtual std::vector<std::string> enumerate() = 0;
        virtual void init(const std::string &devname="") = 0;
        virtual void deinit() = 0;

        virtual Sound *playSound(const std::string &fname, float volume, float pitch, bool loop) = 0;
        virtual Sound *playSound3D(const std::string &fname, const float *pos, float volume, float pitch,
                                   float min, float max, bool loop) = 0;
        virtual Sound *streamSound(const std::string &fname, float volume, float pitch) = 0;
        virtual Sound *streamSound3D(const std::string &fname, const float *pos, float volume, float pitch,
                                     float min, float max) = 0;

        virtual void updateListener(const float *pos, const float *atdir, const float *updir) = 0;

        Sound_Output(SoundManager &mgr) : mManager(mgr) { }
    public:
        virtual ~Sound_Output() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif

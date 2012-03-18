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
        SoundManager &mgr;

        virtual bool Initialize(const std::string &devname="") = 0;
        virtual void Deinitialize() = 0;

        virtual Sound *PlaySound(const std::string &fname, float volume, float pitch, bool loop) = 0;
        virtual Sound *PlaySound3D(const std::string &fname, const float *pos, float volume, float pitch,
                                   float min, float max, bool loop) = 0;
        virtual Sound *StreamSound(const std::string &fname, float volume, float pitch) = 0;

        virtual void UpdateListener(const float *pos, const float *atdir, const float *updir) = 0;

        Sound_Output(SoundManager &mgr) : mgr(mgr) { }
    public:
        virtual ~Sound_Output() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif

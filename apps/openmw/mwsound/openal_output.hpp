#ifndef GAME_SOUND_OPENAL_OUTPUT_H
#define GAME_SOUND_OPENAL_OUTPUT_H

#include <string>

#include "alc.h"
#include "al.h"

#include "sound_output.hpp"

namespace MWSound
{
    class SoundManager;
    class Sound_Decoder;
    class Sound;

    class OpenAL_Output : public Sound_Output
    {
        ALCdevice *mDevice;
        ALCcontext *mContext;

        virtual bool init(const std::string &devname="");
        virtual void deinit();

        virtual Sound *playSound(const std::string &fname, float volume, float pitch, bool loop);
        virtual Sound *playSound3D(const std::string &fname, const float *pos, float volume, float pitch,
                                   float min, float max, bool loop);

        virtual Sound *streamSound(const std::string &fname, float volume, float pitch);

        virtual void updateListener(const float *pos, const float *atdir, const float *updir);

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();

        friend class SoundManager;
    };
#ifndef DEFAULT_OUTPUT
#define DEFAULT_OUTPUT OpenAL_Output
#endif
};

#endif

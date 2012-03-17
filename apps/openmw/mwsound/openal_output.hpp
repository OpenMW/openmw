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
        ALCdevice *Device;
        ALCcontext *Context;

        virtual bool Initialize(const std::string &devname="");
        virtual void Deinitialize();

        virtual Sound *PlaySound(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder,
                                 float volume, float pitch, bool loop);
        virtual Sound *PlaySound3D(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder,
                                   MWWorld::Ptr ptr, float volume, float pitch,
                                   float min, float max, bool loop);

        virtual Sound *StreamSound(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder,
                                   float volume, float pitch);

        virtual void UpdateListener(float pos[3], float atdir[3], float updir[3]);

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();

        friend class SoundManager;
    };
#ifndef DEFAULT_OUTPUT
#define DEFAULT_OUTPUT OpenAL_Output
#endif
};

#endif

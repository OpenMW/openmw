#ifndef GAME_SOUND_MPGSND_DECODER_H
#define GAME_SOUND_MPGSND_DECODER_H

#include <string>

#include "mpg123.h"
#include "sndfile.h"

#include "sound_decoder.hpp"


namespace MWSound
{
    class MpgSnd_Decoder : public Sound_Decoder
    {
        SNDFILE *sndFile;
        mpg123_handle *mpgFile;

        ChannelConfig chanConfig;
        int sampleRate;

        virtual bool Open(const std::string &fname);
        virtual void Close();

        virtual void GetInfo(int *samplerate, ChannelConfig *chans, SampleType *type);
        virtual size_t Read(char *buffer, size_t bytes);

        MpgSnd_Decoder();
        virtual ~MpgSnd_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::MpgSnd_Decoder)
#endif
};

#endif

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
        virtual bool Open(const std::string &fname);
        virtual void Close();

        MpgSnd_Decoder();
        virtual ~MpgSnd_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::MpgSnd_Decoder)
#endif
};

#endif

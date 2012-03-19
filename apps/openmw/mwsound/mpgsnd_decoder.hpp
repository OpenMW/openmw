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
        SNDFILE *mSndFile;
        mpg123_handle *mMpgFile;

        ChannelConfig mChanConfig;
        int mSampleRate;

        virtual void open(const std::string &fname);
        virtual void close();

        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type);

        virtual size_t read(char *buffer, size_t bytes);
        virtual void rewind();

        MpgSnd_Decoder();
    public:
        virtual ~MpgSnd_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::MpgSnd_Decoder)
#endif
};

#endif

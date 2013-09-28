#ifndef GAME_SOUND_AUDIERE_DECODER_H
#define GAME_SOUND_AUDIERE_DECODER_H

#include <OgreDataStream.h>

#include "audiere.h"

#include "sound_decoder.hpp"


namespace MWSound
{
    class Audiere_Decoder : public Sound_Decoder
    {
        audiere::SampleSourcePtr mSoundSource;
        int mSampleRate;
        SampleType mSampleType;
        ChannelConfig mChannelConfig;

        virtual void open(const std::string &fname);
        virtual void close();

        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type);

        virtual size_t read(char *buffer, size_t bytes);
        virtual void rewind();

        Audiere_Decoder& operator=(const Audiere_Decoder &rhs);
        Audiere_Decoder(const Audiere_Decoder &rhs);

        Audiere_Decoder();
    public:
        virtual ~Audiere_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::Audiere_Decoder)
#endif
};

#endif

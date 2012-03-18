#ifndef GAME_SOUND_SOUND_DECODER_H
#define GAME_SOUND_SOUND_DECODER_H

#include <string>

namespace MWSound
{
    enum SampleType {
        SampleType_UInt8,
        SampleType_Int16
    };
    enum ChannelConfig {
        ChannelConfig_Mono,
        ChannelConfig_Stereo
    };

    class Sound_Decoder
    {
    public:
        virtual void open(const std::string &fname) = 0;
        virtual void close() = 0;

        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type) = 0;
        virtual size_t read(char *buffer, size_t bytes) = 0;

        virtual ~Sound_Decoder() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif

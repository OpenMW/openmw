#ifndef GAME_SOUND_SOUND_DECODER_H
#define GAME_SOUND_SOUND_DECODER_H

namespace MWSound
{
    class Sound_Decoder
    {
    public:
        enum SampleType {
            UInt8Sample,
            Int16Sample
        };
        enum ChannelConfig {
            MonoChannels,
            StereoChannels
        };
        virtual void Open(const std::string &fname) = 0;
        virtual void Close() = 0;

        virtual void GetInfo(int *samplerate, ChannelConfig *chans, SampleType *type) = 0;
        virtual size_t Read(char *buffer, size_t bytes) = 0;

        virtual ~Sound_Decoder() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif

#ifndef GAME_SOUND_SOUND_DECODER_H
#define GAME_SOUND_SOUND_DECODER_H

namespace MWSound
{
    class Sound_Decoder
    {
    public:
        virtual bool Open(const std::string &fname) = 0;
        virtual void Close() = 0;

        virtual ~Sound_Decoder() { }

        friend class OpenAL_Output;
        friend class SoundManager;
    };
}

#endif

#ifndef GAME_SOUND_MPGSND_DECODER_H
#define GAME_SOUND_MPGSND_DECODER_H

#include <string>

#include <OgreDataStream.h>

#include "mpg123.h"
#include "sndfile.h"

#include "sound_decoder.hpp"


namespace MWSound
{
    class MpgSnd_Decoder : public Sound_Decoder
    {
        SF_INFO mSndInfo;
        SNDFILE *mSndFile;
        mpg123_handle *mMpgFile;

        Ogre::DataStreamPtr mDataStream;
        static sf_count_t ogresf_get_filelen(void *user_data);
        static sf_count_t ogresf_seek(sf_count_t offset, int whence, void *user_data);
        static sf_count_t ogresf_read(void *ptr, sf_count_t count, void *user_data);
        static sf_count_t ogresf_write(const void*, sf_count_t, void*);
        static sf_count_t ogresf_tell(void *user_data);
        static ssize_t ogrempg_read(void*, void*, size_t);
        static off_t ogrempg_lseek(void*, off_t, int);

        ChannelConfig mChanConfig;
        int mSampleRate;

        virtual void open(const std::string &fname);
        virtual void close();

        virtual std::string getName();
        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type);

        virtual size_t read(char *buffer, size_t bytes);
        virtual void readAll(std::vector<char> &output);
        virtual void rewind();
        virtual size_t getSampleOffset();

        MpgSnd_Decoder& operator=(const MpgSnd_Decoder &rhs);
        MpgSnd_Decoder(const MpgSnd_Decoder &rhs);

        MpgSnd_Decoder();
    public:
        virtual ~MpgSnd_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::MpgSnd_Decoder)
#endif
}

#endif

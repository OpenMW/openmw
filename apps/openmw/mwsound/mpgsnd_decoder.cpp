#ifdef OPENMW_USE_MPG123

#include <stdexcept>
#include <iostream>

#include "mpgsnd_decoder.hpp"


static void fail(const std::string &msg)
{ throw std::runtime_error("MpgSnd exception: "+msg); }

namespace MWSound
{

void MpgSnd_Decoder::open(const std::string &fname)
{
    close();

    SF_INFO info;
    mSndFile = sf_open(fname.c_str(), SFM_READ, &info);
    if(mSndFile)
    {
        if(info.channels == 1)
            mChanConfig = ChannelConfig_Mono;
        else if(info.channels == 2)
            mChanConfig = ChannelConfig_Stereo;
        else
        {
            sf_close(mSndFile);
            mSndFile = NULL;
            fail("Unsupported channel count in "+fname);
        }
        mSampleRate = info.samplerate;
        return;
    }

    mMpgFile = mpg123_new(NULL, NULL);
    if(mMpgFile && mpg123_open(mMpgFile, fname.c_str()) == MPG123_OK)
    {
        try
        {
            int encoding, channels;
            long rate;
            if(mpg123_getformat(mMpgFile, &rate, &channels, &encoding) != MPG123_OK)
                fail("Failed to get audio format");
            if(encoding != MPG123_ENC_SIGNED_16)
                fail("Unsupported encoding in "+fname);
            if(channels != 1 && channels != 2)
                fail("Unsupported channel count in "+fname);
            mChanConfig = ((channels==2)?ChannelConfig_Stereo:ChannelConfig_Mono);
            mSampleRate = rate;
            return;
        }
        catch(std::exception &e)
        {
            mpg123_close(mMpgFile);
            mpg123_delete(mMpgFile);
            mMpgFile = NULL;
            throw;
        }
        mpg123_close(mMpgFile);
    }
    if(mMpgFile)
        mpg123_delete(mMpgFile);
    mMpgFile = NULL;

    fail("Unsupported file type: "+fname);
}

void MpgSnd_Decoder::close()
{
    if(mSndFile)
        sf_close(mSndFile);
    mSndFile = NULL;

    if(mMpgFile)
    {
        mpg123_close(mMpgFile);
        mpg123_delete(mMpgFile);
        mMpgFile = NULL;
    }
}

void MpgSnd_Decoder::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    if(!mSndFile && !mMpgFile)
        fail("No open file");

    *samplerate = mSampleRate;
    *chans = mChanConfig;
    *type = SampleType_Int16;
}

size_t MpgSnd_Decoder::read(char *buffer, size_t bytes)
{
    size_t got = 0;

    if(mSndFile)
    {
        got = sf_read_short(mSndFile, (short*)buffer, bytes/2)*2;
    }
    else if(mMpgFile)
    {
        int err;
        err = mpg123_read(mMpgFile, (unsigned char*)buffer, bytes, &got);
        if(err != MPG123_OK && err != MPG123_DONE)
            fail("Failed to read from file");
    }
    return got;
}

MpgSnd_Decoder::MpgSnd_Decoder() : mSndFile(NULL), mMpgFile(NULL)
{
    static bool initdone = false;
    if(!initdone)
        mpg123_init();
    initdone = true;
}

MpgSnd_Decoder::~MpgSnd_Decoder()
{
    close();
}

}

#endif

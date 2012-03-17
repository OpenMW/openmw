#ifdef OPENMW_USE_MPG123

#include <stdexcept>
#include <iostream>

#include "mpgsnd_decoder.hpp"


static void fail(const std::string &msg)
{ throw std::runtime_error("MpgSnd exception: "+msg); }

namespace MWSound
{

bool MpgSnd_Decoder::Open(const std::string &fname)
{
    Close();

    SF_INFO info;
    sndFile = sf_open(fname.c_str(), SFM_READ, &info);
    if(sndFile)
    {
        if(info.channels == 1)
            chanConfig = MonoChannels;
        else if(info.channels == 1)
            chanConfig = MonoChannels;
        else
        {
            sf_close(sndFile);
            sndFile = NULL;
            fail("Unsupported channel count in "+fname);
        }
        sampleRate = info.samplerate;
        return true;
    }

    mpgFile = mpg123_new(NULL, NULL);
    if(mpgFile && mpg123_open(mpgFile, fname.c_str()) == MPG123_OK)
    {
        try
        {
            int encoding, channels;
            long rate;
            if(mpg123_getformat(mpgFile, &rate, &channels, &encoding) != MPG123_OK)
                fail("Failed to get audio format");
            if(encoding != MPG123_ENC_SIGNED_16)
                fail("Unsupported encoding in "+fname);
            if(channels != 1 && channels != 2)
                fail("Unsupported channel count in "+fname);
            chanConfig = ((channels==2)?StereoChannels:MonoChannels);
            sampleRate = rate;
            return true;
        }
        catch(std::exception &e)
        {
            mpg123_close(mpgFile);
            mpg123_delete(mpgFile);
            throw;
        }
        mpg123_close(mpgFile);
    }
    if(mpgFile)
        mpg123_delete(mpgFile);
    mpgFile = NULL;

    fail("Unsupported file type: "+fname);
    return false;
}

void MpgSnd_Decoder::Close()
{
    if(sndFile)
        sf_close(sndFile);
    sndFile = NULL;

    if(mpgFile)
    {
        mpg123_close(mpgFile);
        mpg123_delete(mpgFile);
        mpgFile = NULL;
    }
}

void MpgSnd_Decoder::GetInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    if(!sndFile && !mpgFile)
        fail("No open file");

    *samplerate = sampleRate;
    *chans = chanConfig;
    *type = Int16Sample;
}

size_t MpgSnd_Decoder::Read(char *buffer, size_t bytes)
{
    size_t got = 0;

    if(sndFile)
    {
        got = sf_read_short(sndFile, (short*)buffer, bytes/2)*2;
    }
    else if(mpgFile)
    {
        int err;
        err = mpg123_read(mpgFile, (unsigned char*)buffer, bytes, &got);
        if(err != MPG123_OK && err != MPG123_DONE)
            fail("Failed to read from file");
    }
    return got;
}

MpgSnd_Decoder::MpgSnd_Decoder() : sndFile(NULL), mpgFile(NULL)
{
    static bool initdone = false;
    if(!initdone)
        mpg123_init();
    initdone = true;
}

MpgSnd_Decoder::~MpgSnd_Decoder()
{
}

}

#endif

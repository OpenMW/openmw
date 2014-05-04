#ifdef OPENMW_USE_MPG123

#include <stdexcept>
#include <iostream>

#include "mpgsnd_decoder.hpp"


static void fail(const std::string &msg)
{ throw std::runtime_error("MpgSnd exception: "+msg); }

namespace MWSound
{

//
// libSndFile io callbacks
//
sf_count_t MpgSnd_Decoder::ogresf_get_filelen(void *user_data)
{
    Ogre::DataStreamPtr stream = static_cast<MpgSnd_Decoder*>(user_data)->mDataStream;
    return stream->size();
}

sf_count_t MpgSnd_Decoder::ogresf_seek(sf_count_t offset, int whence, void *user_data)
{
    Ogre::DataStreamPtr stream = static_cast<MpgSnd_Decoder*>(user_data)->mDataStream;

    if(whence == SEEK_CUR)
        stream->seek(stream->tell()+offset);
    else if(whence == SEEK_SET)
        stream->seek(offset);
    else if(whence == SEEK_END)
        stream->seek(stream->size()+offset);
    else
        return -1;

    return stream->tell();
}

sf_count_t MpgSnd_Decoder::ogresf_read(void *ptr, sf_count_t count, void *user_data)
{
    Ogre::DataStreamPtr stream = static_cast<MpgSnd_Decoder*>(user_data)->mDataStream;
    return stream->read(ptr, count);
}

sf_count_t MpgSnd_Decoder::ogresf_write(const void*, sf_count_t, void*)
{ return -1; }

sf_count_t MpgSnd_Decoder::ogresf_tell(void *user_data)
{
    Ogre::DataStreamPtr stream = static_cast<MpgSnd_Decoder*>(user_data)->mDataStream;
    return stream->tell();
}

//
// libmpg13 io callbacks
//
ssize_t MpgSnd_Decoder::ogrempg_read(void *user_data, void *ptr, size_t count)
{
    Ogre::DataStreamPtr stream = static_cast<MpgSnd_Decoder*>(user_data)->mDataStream;
    return stream->read(ptr, count);
}

off_t MpgSnd_Decoder::ogrempg_lseek(void *user_data, off_t offset, int whence)
{
    Ogre::DataStreamPtr stream = static_cast<MpgSnd_Decoder*>(user_data)->mDataStream;

    if(whence == SEEK_CUR)
        stream->seek(stream->tell()+offset);
    else if(whence == SEEK_SET)
        stream->seek(offset);
    else if(whence == SEEK_END)
        stream->seek(stream->size()+offset);
    else
        return -1;

    return stream->tell();
}


void MpgSnd_Decoder::open(const std::string &fname)
{
    close();
    mDataStream = mResourceMgr.openResource(fname);

    SF_VIRTUAL_IO streamIO = {
        ogresf_get_filelen, ogresf_seek,
        ogresf_read, ogresf_write, ogresf_tell
    };
    mSndFile = sf_open_virtual(&streamIO, SFM_READ, &mSndInfo, this);
    if(mSndFile)
    {
        if(mSndInfo.channels == 1)
            mChanConfig = ChannelConfig_Mono;
        else if(mSndInfo.channels == 2)
            mChanConfig = ChannelConfig_Stereo;
        else
        {
            sf_close(mSndFile);
            mSndFile = NULL;
            fail("Unsupported channel count in "+fname);
        }
        mSampleRate = mSndInfo.samplerate;
        return;
    }
    mDataStream->seek(0);

    mMpgFile = mpg123_new(NULL, NULL);
    if(mMpgFile && mpg123_replace_reader_handle(mMpgFile, ogrempg_read, ogrempg_lseek, NULL) == MPG123_OK &&
       mpg123_open_handle(mMpgFile, this) == MPG123_OK)
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

    mDataStream.setNull();
}

std::string MpgSnd_Decoder::getName()
{
    return mDataStream->getName();
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

void MpgSnd_Decoder::readAll(std::vector<char> &output)
{
    if(mSndFile && mSndInfo.frames > 0)
    {
        size_t pos = output.size();
        output.resize(pos + mSndInfo.frames*mSndInfo.channels*2);
        sf_readf_short(mSndFile, (short*)(&output[0]+pos), mSndInfo.frames);
        return;
    }
    // Fallback in case we don't know the total already
    Sound_Decoder::readAll(output);
}

void MpgSnd_Decoder::rewind()
{
    if(!mSndFile && !mMpgFile)
        fail("No open file");

    if(mSndFile)
    {
        if(sf_seek(mSndFile, 0, SEEK_SET) == -1)
            fail("seek failed");
    }
    else if(mMpgFile)
    {
        if(mpg123_seek(mMpgFile, 0, SEEK_SET) < 0)
            fail("seek failed");
    }
}

size_t MpgSnd_Decoder::getSampleOffset()
{
    return 0;
}

MpgSnd_Decoder::MpgSnd_Decoder()
    : mSndInfo()
    , mSndFile(NULL)
    , mMpgFile(NULL)
    , mDataStream()
    , mChanConfig(ChannelConfig_Stereo)
    , mSampleRate(0)
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

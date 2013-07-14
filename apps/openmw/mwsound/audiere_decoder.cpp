#ifdef OPENMW_USE_AUDIERE

#include <stdexcept>
#include <iostream>

#include "audiere_decoder.hpp"


static void fail(const std::string &msg)
{ throw std::runtime_error("Audiere exception: "+msg); }

namespace MWSound
{

class OgreFile : public audiere::File
{
    Ogre::DataStreamPtr mStream;

    ADR_METHOD(int) read(void* buffer, int size)
    {
        return mStream->read(buffer, size);
    }

    ADR_METHOD(bool) seek(int position, SeekMode mode)
    {
        if(mode == CURRENT)
            mStream->seek(mStream->tell()+position);
        else if(mode == BEGIN)
            mStream->seek(position);
        else if(mode == END)
            mStream->seek(mStream->size()+position);
        else
            return false;

        return true;
    }

    ADR_METHOD(int) tell()
    {
        return mStream->tell();
    }

    size_t refs;
    ADR_METHOD(void) ref() { ++refs; }
    ADR_METHOD(void) unref()
    {
        if(--refs == 0)
            delete this;
    }

public:
    OgreFile(const Ogre::DataStreamPtr &stream)
      : mStream(stream), refs(1)
    { }
    virtual ~OgreFile() { }

    Ogre::String getName()
    { return mStream->getName(); }
};


void Audiere_Decoder::open(const std::string &fname)
{
    close();

    mSoundFile = audiere::FilePtr(new OgreFile(mResourceMgr.openResource(fname)));
    mSoundSource = audiere::OpenSampleSource(mSoundFile);
    mSoundFileName = fname;

    int channels, srate;
    audiere::SampleFormat format;

    mSoundSource->getFormat(channels, srate, format);
    if(format == audiere::SF_S16)
        mSampleType = SampleType_Int16;
    else if(format == audiere::SF_U8)
        mSampleType = SampleType_UInt8;
    else
        fail("Unsupported sample type");

    if(channels == 1)
        mChannelConfig = ChannelConfig_Mono;
    else if(channels == 2)
        mChannelConfig = ChannelConfig_Stereo;
    else
        fail("Unsupported channel count");

    mSampleRate = srate;
}

void Audiere_Decoder::close()
{
    mSoundFile = NULL;
    mSoundSource = NULL;
}

std::string Audiere_Decoder::getName()
{
    return mSoundFileName;
}

void Audiere_Decoder::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    *samplerate = mSampleRate;
    *chans = mChannelConfig;
    *type = mSampleType;
}

size_t Audiere_Decoder::read(char *buffer, size_t bytes)
{
    int size = bytesToFrames(bytes, mChannelConfig, mSampleType);
    size = mSoundSource->read(size, buffer);
    return framesToBytes(size, mChannelConfig, mSampleType);
}

void Audiere_Decoder::rewind()
{
    mSoundSource->reset();
}

size_t Audiere_Decoder::getSampleOffset()
{
    return 0;
}

Audiere_Decoder::Audiere_Decoder()
{
}

Audiere_Decoder::~Audiere_Decoder()
{
    close();
}

}

#endif
